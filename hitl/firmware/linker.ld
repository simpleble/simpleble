SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)
MEMORY
{
  FLASH (rx) : ORIGIN = 0x27000, LENGTH = 0xd9000
  RAM (rwx) : ORIGIN = 0x20006000, LENGTH = 0x3a000
}
SECTIONS
{
  .sdh_soc_observers : { PROVIDE(__start_sdh_soc_observers = .); KEEP(*(SORT(.sdh_soc_observers*))) PROVIDE(__stop_sdh_soc_observers = .); } > FLASH
  .sdh_ble_observers : { PROVIDE(__start_sdh_ble_observers = .); KEEP(*(SORT(.sdh_ble_observers*))) PROVIDE(__stop_sdh_ble_observers = .); } > FLASH
  .sdh_req_observers : { PROVIDE(__start_sdh_req_observers = .); KEEP(*(SORT(.sdh_req_observers*))) PROVIDE(__stop_sdh_req_observers = .); } > FLASH
  .sdh_state_observers : { PROVIDE(__start_sdh_state_observers = .); KEEP(*(SORT(.sdh_state_observers*))) PROVIDE(__stop_sdh_state_observers = .); } > FLASH
  .sdh_stack_observers : { PROVIDE(__start_sdh_stack_observers = .); KEEP(*(SORT(.sdh_stack_observers*))) PROVIDE(__stop_sdh_stack_observers = .); } > FLASH
} INSERT AFTER .text;
INCLUDE "nrf_common.ld"
