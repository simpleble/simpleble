import { Arrow, Box, LearnFigure, Label } from './figure';

function Bracket({ x1, x2, y, label, tone = 'muted' }: { x1: number; x2: number; y: number; label: string; tone?: 'muted' | 'primary' }) {
  const stroke = tone === 'primary' ? 'stroke-primary' : 'stroke-fd-muted-foreground';
  return (
    <g>
      <path d={`M${x1},${y + 5} V${y} H${x2} V${y + 5}`} fill="none" strokeWidth={1.25} className={stroke} />
      <Label x={(x1 + x2) / 2} y={y - 6} size={11.5} tone={tone}>
        {label}
      </Label>
    </g>
  );
}

export function AdvertisingEventDiagram() {
  const events = [60, 220, 380];
  const channels = ['37', '38', '39'];

  return (
    <LearnFigure
      title="Advertising events repeating on channels 37, 38 and 39"
      caption="Each advertising event sends the same advertisement on channels 37, 38 and 39. Events repeat at the advertising interval, plus a random delay of up to 10 ms."
      width={460}
      height={150}
    >
      <line x1={8} y1={96} x2={452} y2={96} strokeWidth={1} className="stroke-fd-border" />
      {events.map((start, eventIndex) => (
        <g key={start}>
          {channels.map((channel, index) => (
            <g key={channel}>
              <rect
                x={start + index * 22}
                y={70}
                width={12}
                height={26}
                rx={2}
                className={eventIndex === 0 ? 'fill-primary' : 'fill-fd-foreground'}
              />
              <Label x={start + index * 22 + 6} y={114} size={10.5} tone="muted">
                {channel}
              </Label>
            </g>
          ))}
        </g>
      ))}
      <Label x={20} y={114} size={10.5} tone="muted" anchor="start">
        channel
      </Label>
      <Bracket x1={60} x2={220} y={50} label="advertising interval + delay" />
      <Bracket x1={220} x2={380} y={50} label="advertising interval + delay" />
      <Label x={88} y={138} size={12} tone="primary" weight={600}>
        one advertising event
      </Label>
    </LearnFigure>
  );
}

export function IBeaconLayoutDiagram() {
  const segments = [
    { w: 56, bytes: '4C 00', name: 'company', value: 'Apple', tone: 'muted' as const },
    { w: 36, bytes: '02', name: 'type', value: 'iBeacon', tone: 'primary' as const },
    { w: 36, bytes: '15', name: 'length', value: '21', tone: 'primary' as const },
    { w: 138, bytes: 'E2 C5 6D B5 … 96 E0', name: 'proximity UUID', value: '16 bytes', tone: 'default' as const },
    { w: 50, bytes: '00 01', name: 'major', value: '1', tone: 'default' as const },
    { w: 50, bytes: '00 02', name: 'minor', value: '2', tone: 'default' as const },
    { w: 34, bytes: 'C5', name: 'power', value: '-59 dBm', tone: 'default' as const },
  ];
  const gap = 2;
  const startX = 24;
  const positions = segments.reduce<number[]>((list, segment, index) => {
    list.push(index === 0 ? startX : list[index - 1] + segments[index - 1].w + gap);
    return list;
  }, []);
  const dataStart = positions[1];
  const dataEnd = positions[positions.length - 1] + segments[segments.length - 1].w;

  return (
    <LearnFigure
      title="The bytes of an iBeacon advertisement's manufacturer data"
      caption="An iBeacon is Apple manufacturer data whose payload starts with type 0x02 and length 0x15. SimpleBLE gives you the company identifier as the key and the 23 bytes after it as the data."
      width={460}
      height={150}
    >
      <Bracket x1={startX} x2={startX + segments[0].w} y={28} label="key" />
      <Bracket x1={dataStart} x2={dataEnd} y={28} label="data, 23 bytes" tone="primary" />
      {segments.map((segment, index) => {
        const x = positions[index];
        return (
          <g key={segment.name}>
            <rect
              x={x}
              y={44}
              width={segment.w}
              height={32}
              rx={4}
              strokeWidth={1.25}
              className={
                segment.tone === 'muted'
                  ? 'fill-fd-muted stroke-fd-border'
                  : segment.tone === 'primary'
                    ? 'fill-primary/10 stroke-primary'
                    : 'fill-fd-card stroke-fd-border'
              }
            />
            <Label x={x + segment.w / 2} y={64.5} size={11.5} mono>
              {segment.bytes}
            </Label>
            <Label x={x + segment.w / 2} y={98} size={11.5} weight={600}>
              {segment.name}
            </Label>
            <Label x={x + segment.w / 2} y={115} size={11} tone="muted">
              {segment.value}
            </Label>
          </g>
        );
      })}
    </LearnFigure>
  );
}

export function CoursePeripheralDiagram() {
  const characteristics = [
    { y: 112, name: 'Message', uuid: '76a60002-…', properties: 'read, write' },
    { y: 172, name: 'Counter', uuid: '76a60003-…', properties: 'read, notify, indicate' },
  ];

  return (
    <LearnFigure
      title="The course peripheral: your computer serves one service with two characteristics to a phone"
      caption="Your computer advertises the course service so a central can find it. Once a phone connects, it reads and writes Message and subscribes to Counter."
      width={460}
      height={252}
    >
      <Box x={6} y={6} w={262} h={240} tone="muted" dashed />
      <Label x={20} y={28} size={12} tone="muted" anchor="start">
        Your computer: peripheral
      </Label>
      <Box x={18} y={42} w={238} h={192} tone="primary" />
      <Label x={32} y={64} size={13} weight={600} tone="primary" anchor="start">
        Course service
      </Label>
      <Label x={32} y={81} size={11.5} tone="muted" anchor="start" mono>
        76a60001-a8e9-4660-…
      </Label>
      {characteristics.map((characteristic) => (
        <g key={characteristic.name}>
          <Box x={30} y={characteristic.y - 10} w={214} h={50} />
          <Label x={42} y={characteristic.y + 10} size={13} weight={600} anchor="start">
            {characteristic.name}
          </Label>
          <Label x={232} y={characteristic.y + 10} size={11} tone="muted" anchor="end" mono>
            {characteristic.uuid}
          </Label>
          <Label x={42} y={characteristic.y + 29} size={11.5} tone="primary" anchor="start">
            {characteristic.properties}
          </Label>
        </g>
      ))}
      <Box x={330} y={96} w={122} h={60} />
      <Label x={391} y={121} weight={600}>
        Phone
      </Label>
      <Label x={391} y={139} size={11.5} tone="muted">
        central
      </Label>
      <Arrow x1={270} y1={66} x2={330} y2={104} dashed />
      <Label x={322} y={66} size={11} tone="muted" anchor="start">
        advertises
      </Label>
      <Arrow x1={256} y1={137} x2={328} y2={137} tone="primary" both />
      <Arrow x1={256} y1={197} x2={350} y2={158} tone="primary" both />
      <Label x={360} y={194} size={11} tone="primary" anchor="start">
        read, write,
      </Label>
      <Label x={360} y={208} size={11} tone="primary" anchor="start">
        subscribe
      </Label>
    </LearnFigure>
  );
}

export function DiscoveryTimingDiagram() {
  const start = 100;
  const events = [142, 225, 301, 384];
  const windows = [100, 190, 280, 370];
  const windowWidth = 30;
  const channels = ['37', '38', '39', '37'];
  const found = events.find((x) => windows.some((w) => x >= w && x <= w + windowWidth)) ?? events[0];

  return (
    <LearnFigure
      title="Advertising events and scan windows on one timeline, with the first overlap marked as discovery"
      caption="The scanner listens on one channel at a time, for a scan window once every scan interval. The device is discovered when one of its advertising events lands inside a window."
      width={460}
      height={236}
    >
      <Label x={10} y={88} size={13} anchor="start" weight={600}>
        Advertiser
      </Label>
      <Label x={10} y={148} size={13} anchor="start" weight={600}>
        Scanner
      </Label>
      <line x1={start} y1={94} x2={452} y2={94} strokeWidth={1} className="stroke-fd-border" />
      <line x1={start} y1={154} x2={452} y2={154} strokeWidth={1} className="stroke-fd-border" />
      <Bracket x1={events[0]} x2={events[1]} y={50} label="advertising interval" />
      {events.map((x) => (
        <rect
          key={x}
          x={x - 4}
          y={70}
          width={8}
          height={24}
          rx={2}
          className={x === found ? 'fill-primary' : 'fill-fd-foreground'}
        />
      ))}
      {windows.map((x, index) => (
        <g key={x}>
          <rect
            x={x}
            y={130}
            width={windowWidth}
            height={24}
            rx={3}
            strokeWidth={1}
            className="fill-fd-muted stroke-fd-muted-foreground"
          />
          <Label x={x + windowWidth / 2} y={146.5} size={10.5} tone="muted">
            {channels[index]}
          </Label>
        </g>
      ))}
      <Bracket x1={windows[1]} x2={windows[2]} y={118} label="scan interval" />
      <line x1={found} y1={94} x2={found} y2={130} strokeWidth={1.25} strokeDasharray="3 3" className="stroke-primary" />
      <line x1={found} y1={154} x2={found} y2={196} strokeWidth={1.25} strokeDasharray="3 3" className="stroke-primary" />
      <line x1={start} y1={64} x2={start} y2={196} strokeWidth={1.25} strokeDasharray="3 3" className="stroke-fd-muted-foreground" />
      <Label x={start} y={210} size={11.5} tone="muted">
        scan starts
      </Label>
      <Arrow x1={start + 2} y1={190} x2={found - 2} y2={190} tone="primary" both />
      <Label x={(start + found) / 2} y={182} size={11.5} tone="primary" weight={600}>
        discovery time
      </Label>
      <Label x={found} y={210} size={11.5} tone="primary" weight={600}>
        found
      </Label>
      <Label x={windows[3] + windowWidth / 2} y={176} size={11} tone="muted">
        scan window
      </Label>
    </LearnFigure>
  );
}
