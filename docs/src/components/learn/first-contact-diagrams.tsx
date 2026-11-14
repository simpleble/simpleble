import { Arrow, Box, LearnFigure, Label } from './figure';

export function BleStackDiagram() {
  const layers = [
    { title: 'Your program', detail: 'the code in each lesson', tone: 'default' as const },
    { title: 'SimpleBLE', detail: 'one API in every language', tone: 'primary' as const },
    { title: 'OS Bluetooth stack', detail: 'WinRT, CoreBluetooth, BlueZ, Android', tone: 'default' as const },
    { title: 'Bluetooth adapter', detail: 'built-in radio or USB dongle', tone: 'default' as const },
  ];

  return (
    <LearnFigure
      title="Layers between a program and a nearby Bluetooth Low Energy device"
      caption="Your program calls SimpleBLE, SimpleBLE drives the operating system's Bluetooth stack, and the adapter's radio talks to nearby devices."
      width={460}
      height={312}
    >
      <Box x={6} y={6} w={252} h={300} tone="muted" dashed />
      <Label x={20} y={28} size={12} tone="muted" anchor="start">
        Your computer
      </Label>
      {layers.map((layer, index) => {
        const y = 40 + index * 68;
        return (
          <g key={layer.title}>
            <Box x={18} y={y} w={228} h={48} tone={layer.tone} />
            <Label x={132} y={y + 21} weight={600} tone={layer.tone === 'primary' ? 'primary' : 'default'}>
              {layer.title}
            </Label>
            <Label x={132} y={y + 38} size={11.5} tone="muted">
              {layer.detail}
            </Label>
            {index < layers.length - 1 ? <Arrow x1={132} y1={y + 48} x2={132} y2={y + 68} both /> : null}
          </g>
        );
      })}
      <Box x={330} y={244} w={122} h={48} />
      <Label x={391} y={265} weight={600}>
        BLE device
      </Label>
      <Label x={391} y={282} size={11.5} tone="muted">
        phone, sensor, beacon
      </Label>
      <Arrow x1={248} y1={268} x2={328} y2={268} tone="primary" dashed both />
      <Label x={289} y={258} size={12} tone="primary">
        2.4 GHz
      </Label>
    </LearnFigure>
  );
}

export function RolesSequenceDiagram() {
  const messages = [
    { y: 100, label: 'Advertisement', fromPeripheral: true, dashed: true },
    { y: 140, label: 'Advertisement', fromPeripheral: true, dashed: true },
    { y: 180, label: 'Connection request', fromPeripheral: false, dashed: false },
    { y: 220, label: 'Read or write', fromPeripheral: false, dashed: false },
    { y: 260, label: 'Notification', fromPeripheral: true, dashed: false },
  ];

  return (
    <LearnFigure
      title="A peripheral advertises, a central connects, and the two exchange data"
      caption="Advertisements travel one way until the central connects. After that, data moves in both directions."
      width={460}
      height={290}
    >
      <Box x={20} y={10} w={160} h={50} />
      <Label x={100} y={32} weight={600}>
        Peripheral
      </Label>
      <Label x={100} y={49} size={11.5} tone="muted">
        advertises, serves data
      </Label>
      <Box x={280} y={10} w={160} h={50} tone="primary" />
      <Label x={360} y={32} weight={600} tone="primary">
        Central
      </Label>
      <Label x={360} y={49} size={11.5} tone="muted">
        scans, connects
      </Label>
      <line x1={100} y1={60} x2={100} y2={282} strokeWidth={1.5} strokeDasharray="3 4" className="stroke-fd-border" />
      <line x1={360} y1={60} x2={360} y2={282} strokeWidth={1.5} strokeDasharray="3 4" className="stroke-fd-border" />
      {messages.map((message) => (
        <g key={`${message.y}`}>
          <Label x={230} y={message.y - 8} size={13}>
            {message.label}
          </Label>
          <Arrow
            x1={message.fromPeripheral ? 102 : 358}
            y1={message.y}
            x2={message.fromPeripheral ? 358 : 102}
            y2={message.y}
            tone={message.fromPeripheral ? 'default' : 'primary'}
            dashed={message.dashed}
          />
        </g>
      ))}
    </LearnFigure>
  );
}

export function ScanTimelineDiagram() {
  const packets = [128, 182, 236, 300, 367, 420];
  const windows = [110, 190, 270, 350, 430];
  const windowWidth = 40;
  const heard = packets.filter((x) => windows.some((start) => x >= start && x <= start + windowWidth));

  return (
    <LearnFigure
      title="An advertiser repeats its advertisement while the scanner listens in windows"
      caption="The adapter hears an advertisement when it arrives during a scan window. The first one heard reports the device as found. Every later one reports an update."
      width={460}
      height={206}
    >
      <Label x={10} y={54} size={13} anchor="start" weight={600}>
        Advertiser
      </Label>
      <Label x={10} y={114} size={13} anchor="start" weight={600}>
        Scanner
      </Label>
      <Label x={10} y={174} size={13} anchor="start" weight={600}>
        Your program
      </Label>
      <line x1={108} y1={60} x2={452} y2={60} strokeWidth={1} className="stroke-fd-border" />
      <line x1={108} y1={120} x2={452} y2={120} strokeWidth={1} className="stroke-fd-border" />
      <line x1={108} y1={180} x2={452} y2={180} strokeWidth={1} className="stroke-fd-border" />
      {windows.map((start) => (
        <rect
          key={start}
          x={start}
          y={100}
          width={Math.min(windowWidth, 452 - start)}
          height={20}
          rx={3}
          strokeWidth={1}
          className="fill-fd-muted stroke-fd-muted-foreground"
        />
      ))}
      {heard.map((x, index) => (
        <g key={`heard-${x}`}>
          <line x1={x} y1={60} x2={x} y2={166} strokeWidth={1.25} strokeDasharray="3 3" className="stroke-primary" />
          <circle cx={x} cy={180} r={4.5} className="fill-primary" />
          <Label x={x} y={198} size={12} tone="primary" weight={600}>
            {index === 0 ? 'found' : 'updated'}
          </Label>
        </g>
      ))}
      {packets.map((x) => (
        <rect key={x} x={x - 4} y={38} width={8} height={22} rx={2} className="fill-fd-foreground" />
      ))}
      <Label x={128} y={28} size={11} tone="muted">
        advertisement
      </Label>
      <Label x={310} y={92} size={11} tone="muted">
        scan window
      </Label>
    </LearnFigure>
  );
}

export function AdvertisingStructuresDiagram() {
  const rows = [
    { bytes: ['02', '01', '06'], name: 'Flags', value: 'discoverable, LE only', call: '' },
    { bytes: ['03', '03', '0F', '18'], name: 'Service UUID 0x180F', value: '', call: 'services()' },
    { bytes: ['02', '0A', '04'], name: 'TX power: 4 dBm', value: '', call: 'tx_power()' },
    { bytes: ['07', '09', '53', '65', '6E', '73', '6F', '72'], name: 'Name: "Sensor"', value: '', call: 'identifier()' },
    { bytes: ['05', 'FF', 'FF', 'FF', '01', '02'], name: 'Company 0xFFFF: 01 02', value: '', call: 'manufacturer_data()' },
  ];
  const cell = 32;
  const step = cell + 2;
  const labelX = 16 + 8 * step + 10;

  return (
    <LearnFigure
      title="An advertisement split into length, type and data structures"
      caption="Each structure starts with its length and its type. SimpleBLE decodes the structures and returns each one through its own call."
      width={470}
      height={262}
    >
      <Label x={16 + cell / 2} y={26} size={11} tone="muted">
        length
      </Label>
      <Label x={16 + step + cell / 2} y={26} size={11} tone="muted">
        type
      </Label>
      <Label x={16 + step * 2 + 4} y={26} size={11} tone="muted" anchor="start">
        data
      </Label>
      {rows.map((row, rowIndex) => {
        const y = 36 + rowIndex * 44;
        return (
          <g key={row.name}>
            {row.bytes.map((byte, index) => (
              <g key={`${row.name}-${index}`}>
                <rect
                  x={16 + index * step}
                  y={y}
                  width={cell}
                  height={28}
                  rx={4}
                  strokeWidth={1.25}
                  className={
                    index === 0
                      ? 'fill-fd-muted stroke-fd-border'
                      : index === 1
                        ? 'fill-primary/10 stroke-primary'
                        : 'fill-fd-card stroke-fd-border'
                  }
                />
                <Label x={16 + index * step + cell / 2} y={y + 18.5} size={12} mono>
                  {byte}
                </Label>
              </g>
            ))}
            <Label x={labelX} y={y + 12} size={13} anchor="start" weight={600}>
              {row.name}
            </Label>
            <Label x={labelX} y={y + 28} size={12} anchor="start" tone={row.call ? 'primary' : 'muted'} mono={!!row.call}>
              {row.call || row.value}
            </Label>
          </g>
        );
      })}
    </LearnFigure>
  );
}

function seededNoise(count: number) {
  let state = 7;
  const values: number[] = [];
  for (let index = 0; index < count; index++) {
    state = (state * 1103515245 + 12345) % 2147483648;
    values.push((state / 2147483648) * 2 - 1);
  }
  return values;
}

export function RssiTraceDiagram() {
  const left = 52;
  const right = 446;
  const top = 30;
  const bottom = 200;
  const seconds = 15;
  const maxDbm = -40;
  const minDbm = -90;
  const toX = (t: number) => left + (t / seconds) * (right - left);
  const toY = (dbm: number) => top + ((maxDbm - dbm) / (maxDbm - minDbm)) * (bottom - top);

  const noise = seededNoise(46);
  const samples = noise.map((offset, index) => {
    const t = 0.15 + index * 0.32;
    const level = t < 8 ? -56 : t < 9 ? -56 - (t - 8) * 18 : -74;
    return { t, dbm: level + offset * 6 };
  });
  const average = samples.map((sample, index) => {
    const window = samples.slice(Math.max(0, index - 4), index + 1);
    return { t: sample.t, dbm: window.reduce((sum, item) => sum + item.dbm, 0) / window.length };
  });
  const path = average.map((point, index) => `${index === 0 ? 'M' : 'L'}${toX(point.t)},${toY(point.dbm)}`).join(' ');

  return (
    <LearnFigure
      title="RSSI readings over fifteen seconds, before and after a phone moves away"
      caption="An illustration of typical readings. Each dot is one advertisement. Single readings jump by several dB, and the average shows the real change when the phone moves to the next room."
      width={460}
      height={236}
    >
      {[-40, -50, -60, -70, -80, -90].map((dbm) => (
        <g key={dbm}>
          <line x1={left} y1={toY(dbm)} x2={right} y2={toY(dbm)} strokeWidth={1} className="stroke-fd-border" />
          <Label x={left - 6} y={toY(dbm) + 4} size={11} anchor="end" tone="muted">
            {`${dbm}`}
          </Label>
        </g>
      ))}
      {[0, 5, 10, 15].map((t) => (
        <Label key={t} x={toX(t)} y={bottom + 18} size={11} tone="muted">
          {`${t} s`}
        </Label>
      ))}
      <Label x={left - 6} y={14} size={11} anchor="end" tone="muted">
        dBm
      </Label>
      {samples.map((sample) => (
        <circle key={sample.t} cx={toX(sample.t)} cy={toY(sample.dbm)} r={3} className="fill-fd-muted-foreground/70" />
      ))}
      <path d={path} fill="none" strokeWidth={2.5} strokeLinejoin="round" className="stroke-primary" />
      <Label x={toX(0.3)} y={toY(-44)} size={12} anchor="start">
        Phone next to the computer
      </Label>
      <Label x={toX(14.7)} y={toY(-84)} size={12} anchor="end">
        Phone in the next room
      </Label>
      <Label x={toX(14.7)} y={toY(-63)} size={12} anchor="end" tone="primary" weight={600}>
        average
      </Label>
    </LearnFigure>
  );
}
