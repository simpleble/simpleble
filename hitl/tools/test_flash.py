"""Probe selection and target validation without hardware."""

import unittest
from types import SimpleNamespace
from unittest.mock import patch

from flash import attach


class AttachTests(unittest.TestCase):
    def setUp(self):
        factory = patch("flash.pylink.JLink")
        self.link = factory.start().return_value
        self.addCleanup(factory.stop)
        self.link.connected_emulators.return_value = [SimpleNamespace(SerialNumber=123)]
        self.link.serial_number = 123
        self.link.memory_read32.side_effect = [
            [0x52840],
            [0x99E294C3, 0xAFCF4A59],
        ]
        self.link.halted.return_value = False

    def test_discovers_probe_and_board(self):
        link, probe, board = attach()
        self.assertIs(link, self.link)
        self.assertEqual(probe, 123)
        self.assertEqual(board, "afcf4a5999e294c3")
        self.link.open.assert_called_once_with(serial_no=123)
        self.link.halt.assert_not_called()
        self.link.reset.assert_not_called()
        self.link.close.assert_not_called()

    def test_explicit_probe_and_board(self):
        _, probe, board = attach(123, "AFCF4A5999E294C3")
        self.assertEqual((probe, board), (123, "afcf4a5999e294c3"))
        self.link.connected_emulators.assert_not_called()
        self.link.open.assert_called_once_with(serial_no=123)

    def test_requires_one_probe_for_discovery(self):
        for serials, message in [
            ([], "No USB J-Link probe found"),
            (
                [123, 456],
                "Multiple J-Link probes found (123, 456); select one with --probe",
            ),
        ]:
            with self.subTest(serials=serials):
                self.link.connected_emulators.return_value = [
                    SimpleNamespace(SerialNumber=serial) for serial in serials
                ]
                with self.assertRaises(RuntimeError) as error:
                    attach()
                self.assertEqual(str(error.exception), message)
                self.link.open.assert_not_called()
                self.link.close.assert_called()

    def test_rejects_wrong_part_board_or_halted_target(self):
        for part, board, halted, message in [
            (0x52832, None, False, "Wrong target"),
            (0x52840, "0000000000000000", False, "Wrong target"),
            (0x52840, None, True, "Target is halted"),
        ]:
            with self.subTest(part=part, board=board, halted=halted):
                self.link.memory_read32.side_effect = [
                    [part],
                    [0x99E294C3, 0xAFCF4A59],
                ]
                self.link.halted.return_value = halted
                with self.assertRaisesRegex(RuntimeError, message):
                    attach(board=board)
                self.link.close.assert_called()


if __name__ == "__main__":
    unittest.main()
