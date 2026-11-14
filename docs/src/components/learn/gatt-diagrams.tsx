import { Arrow, Box, LearnFigure, Label } from './figure';

type Message = { y: number; label: string; toCentral: boolean; dashed?: boolean; muted?: boolean };

function Lifelines({
  left,
  right,
  top,
  bottom,
  leftTitle,
  leftDetail,
  rightTitle,
  rightDetail,
}: {
  left: number;
  right: number;
  top: number;
  bottom: number;
  leftTitle: string;
  leftDetail: string;
  rightTitle: string;
  rightDetail: string;
}) {
  return (
    <g>
      <Box x={left - 70} y={top} w={140} h={46} />
      <Label x={left} y={top + 20} weight={600}>
        {leftTitle}
      </Label>
      <Label x={left} y={top + 37} size={11.5} tone="muted">
        {leftDetail}
      </Label>
      <Box x={right - 70} y={top} w={140} h={46} tone="primary" />
      <Label x={right} y={top + 20} weight={600} tone="primary">
        {rightTitle}
      </Label>
      <Label x={right} y={top + 37} size={11.5} tone="muted">
        {rightDetail}
      </Label>
      <line x1={left} y1={top + 46} x2={left} y2={bottom} strokeWidth={1.5} strokeDasharray="3 4" className="stroke-fd-border" />
      <line x1={right} y1={top + 46} x2={right} y2={bottom} strokeWidth={1.5} strokeDasharray="3 4" className="stroke-fd-border" />
    </g>
  );
}

function Messages({ messages, left, right }: { messages: Message[]; left: number; right: number }) {
  return (
    <g>
      {messages.map((message) => (
        <g key={`${message.y}-${message.label}`}>
          <Label x={(left + right) / 2} y={message.y - 7} size={12.5} tone={message.muted ? 'muted' : 'default'}>
            {message.label}
          </Label>
          <Arrow
            x1={message.toCentral ? left + 2 : right - 2}
            y1={message.y}
            x2={message.toCentral ? right - 2 : left + 2}
            y2={message.y}
            tone={message.muted ? 'muted' : message.toCentral ? 'default' : 'primary'}
            dashed={message.dashed}
          />
        </g>
      ))}
    </g>
  );
}

function Note({ x, y, children, tone = 'primary' }: { x: number; y: number; children: string; tone?: 'primary' | 'muted' }) {
  return (
    <Label x={x} y={y} size={11.5} tone={tone} anchor="start" mono={tone === 'primary'}>
      {children}
    </Label>
  );
}

export function ConnectionSequenceDiagram() {
  const left = 80;
  const right = 320;
  const messages: Message[] = [
    { y: 92, label: 'Advertisement', toCentral: true, dashed: true },
    { y: 132, label: 'Connection request', toCentral: false },
    { y: 178, label: 'Discover services', toCentral: false },
    { y: 214, label: 'Services and characteristics', toCentral: true },
    { y: 260, label: 'Reads, writes, notifications', toCentral: true, muted: true },
    { y: 306, label: 'Disconnect', toCentral: false },
  ];

  return (
    <LearnFigure
      title="A central connects to a peripheral, discovers its services, exchanges data and disconnects"
      caption="connect() opens the connection and discovers the services, and returns once both are done. The two devices then exchange data until one of them disconnects."
      width={500}
      height={326}
    >
      <Lifelines
        left={left}
        right={right}
        top={8}
        bottom={318}
        leftTitle="Peripheral"
        leftDetail="course peripheral"
        rightTitle="Central"
        rightDetail="your program"
      />
      <Messages messages={messages} left={left} right={right} />
      <path d={`M${right + 8},132 h6 v82 h-6`} fill="none" strokeWidth={1.25} className="stroke-primary" />
      <Note x={right + 20} y={177}>
        connect()
      </Note>
      <Note x={right + 20} y={310}>
        disconnect()
      </Note>
      <Note x={right + 20} y={264} tone="muted">
        connected
      </Note>
    </LearnFigure>
  );
}

export function GattTableDiagram() {
  const rows = [
    { handle: '0x0010', type: 'Service declaration', value: '76a60001-…', level: 0, tone: 'primary' as const },
    { handle: '0x0011', type: 'Characteristic declaration', value: 'read, write, write without response', level: 1, tone: 'default' as const },
    { handle: '0x0012', type: 'Message value', value: 'Hello from SimpleBLE', level: 1, tone: 'default' as const },
    { handle: '0x0013', type: 'Characteristic declaration', value: 'read, notify, indicate', level: 1, tone: 'default' as const },
    { handle: '0x0014', type: 'Counter value', value: '05 00 00 00', level: 1, tone: 'default' as const },
    { handle: '0x0015', type: 'CCCD descriptor 0x2902', value: '01 00', level: 2, tone: 'muted' as const },
  ];
  const top = 34;
  const rowHeight = 34;

  return (
    <LearnFigure
      title="The course peripheral's GATT table, attribute by attribute"
      caption="A GATT table is a list of attributes, each with a handle, a type and a value. A service declaration opens each service, every characteristic has a declaration and a value, and descriptors follow the value. The handles here are examples."
      width={556}
      height={top + rows.length * rowHeight + 10}
    >
      <Label x={20} y={22} size={11} tone="muted" anchor="start">
        handle
      </Label>
      <Label x={96} y={22} size={11} tone="muted" anchor="start">
        type
      </Label>
      <Label x={290} y={22} size={11} tone="muted" anchor="start">
        value
      </Label>
      {rows.map((row, index) => {
        const y = top + index * rowHeight;
        const indent = row.level * 12;
        return (
          <g key={row.handle}>
            <rect
              x={88 + indent}
              y={y}
              width={460 - indent}
              height={rowHeight - 6}
              rx={5}
              strokeWidth={1.25}
              className={
                row.tone === 'primary'
                  ? 'fill-primary/10 stroke-primary'
                  : row.tone === 'muted'
                    ? 'fill-fd-muted stroke-fd-border'
                    : 'fill-fd-card stroke-fd-border'
              }
            />
            <Label x={20} y={y + 18} size={11.5} tone="muted" anchor="start" mono>
              {row.handle}
            </Label>
            <Label x={96 + indent} y={y + 18} size={12} weight={600} anchor="start" tone={row.tone === 'primary' ? 'primary' : 'default'}>
              {row.type}
            </Label>
            <Label x={290} y={y + 18} size={11} anchor="start" mono tone="muted">
              {row.value}
            </Label>
          </g>
        );
      })}
    </LearnFigure>
  );
}

export function UuidExpansionDiagram() {
  const mono = 'font-mono';
  return (
    <LearnFigure
      title="A 16-bit UUID expanded into the Bluetooth Base UUID"
      caption="A 16-bit UUID takes the place of xxxx in the Bluetooth Base UUID. Both forms name the same characteristic."
      width={460}
      height={196}
    >
      <Label x={20} y={36} size={12} tone="muted" anchor="start">
        16-bit
      </Label>
      <Box x={110} y={14} w={86} h={34} tone="primary" />
      <Label x={153} y={36} size={14} tone="primary" mono>
        0x2A37
      </Label>
      <Label x={20} y={102} size={12} tone="muted" anchor="start">
        Base UUID
      </Label>
      <Box x={110} y={80} w={340} h={34} />
      <text x={122} y={102} fontSize={14} className={`fill-fd-muted-foreground ${mono}`}>
        0000<tspan className="fill-primary font-semibold">xxxx</tspan>-0000-1000-8000-00805f9b34fb
      </text>
      <Label x={20} y={168} size={12} tone="muted" anchor="start">
        128-bit
      </Label>
      <Box x={110} y={146} w={340} h={34} tone="primary" />
      <text x={122} y={168} fontSize={14} className={`fill-fd-foreground ${mono}`}>
        0000<tspan className="fill-primary font-semibold">2a37</tspan>-0000-1000-8000-00805f9b34fb
      </text>
      <Arrow x1={153} y1={50} x2={153} y2={78} tone="primary" />
      <Arrow x1={153} y1={116} x2={153} y2={144} tone="primary" />
    </LearnFigure>
  );
}

export function ReadWriteDiagram() {
  const left = 80;
  const right = 320;
  const messages: Message[] = [
    { y: 92, label: 'Read request', toCentral: false },
    { y: 126, label: 'Read response: Hello from SimpleBLE', toCentral: true },
    { y: 182, label: 'Write request: hi', toCentral: false },
    { y: 216, label: 'Write response', toCentral: true },
    { y: 272, label: 'Write command: hi', toCentral: false },
  ];

  return (
    <LearnFigure
      title="A read, a write request and a write command between a central and a peripheral"
      caption="A read and a write request each wait for the peripheral's response. A write command expects none, so the central can send the next one right away."
      width={500}
      height={300}
    >
      <Lifelines
        left={left}
        right={right}
        top={8}
        bottom={292}
        leftTitle="Peripheral"
        leftDetail="course peripheral"
        rightTitle="Central"
        rightDetail="your program"
      />
      <Messages messages={messages} left={left} right={right} />
      <Note x={right + 16} y={113}>
        read()
      </Note>
      <Note x={right + 16} y={203}>
        write_request()
      </Note>
      <Note x={right + 16} y={276}>
        write_command()
      </Note>
    </LearnFigure>
  );
}

export function NotifyIndicateDiagram() {
  const columns = [
    { title: 'Notify', left: 40, right: 200, ack: false },
    { title: 'Indicate', left: 290, right: 450, ack: true },
  ];

  return (
    <LearnFigure
      title="Notifications arrive without replies; each indication waits for a confirmation"
      caption="The peripheral sends notifications as values change, with no reply. It sends an indication only after the central confirmed the previous one."
      width={500}
      height={262}
    >
      {columns.map((column) => {
        const steps = column.ack
          ? [
              { y: 92, label: 'Counter: 5', toCentral: true },
              { y: 124, label: 'Confirmation', toCentral: false },
              { y: 172, label: 'Counter: 6', toCentral: true },
              { y: 204, label: 'Confirmation', toCentral: false },
            ]
          : [
              { y: 92, label: 'Counter: 5', toCentral: true },
              { y: 140, label: 'Counter: 6', toCentral: true },
              { y: 188, label: 'Counter: 7', toCentral: true },
            ];
        return (
          <g key={column.title}>
            <Label x={(column.left + column.right) / 2} y={18} weight={600} tone="primary">
              {column.title}
            </Label>
            <Label x={column.left} y={46} size={11.5} tone="muted">
              peripheral
            </Label>
            <Label x={column.right} y={46} size={11.5} tone="muted">
              central
            </Label>
            <line x1={column.left} y1={56} x2={column.left} y2={250} strokeWidth={1.5} strokeDasharray="3 4" className="stroke-fd-border" />
            <line x1={column.right} y1={56} x2={column.right} y2={250} strokeWidth={1.5} strokeDasharray="3 4" className="stroke-fd-border" />
            <Messages
              messages={steps.map((step) => ({ ...step, toCentral: step.toCentral }))}
              left={column.left}
              right={column.right}
            />
          </g>
        );
      })}
    </LearnFigure>
  );
}

export function MtuPacketsDiagram() {
  const scale = 3.4;
  const rows = [
    { y: 52, title: 'ATT MTU 23', packets: [20, 20, 20] },
    { y: 132, title: 'ATT MTU 247', packets: [60] },
  ];

  return (
    <LearnFigure
      title="A 60-byte value sent in writes at two MTU sizes"
      caption="Every write and notification carries 3 bytes of ATT header, so one write holds up to MTU minus 3 bytes. At the default MTU of 23, a 60-byte value takes three writes. After an MTU exchange, it fits in one."
      width={460}
      height={186}
    >
      {rows.map((row) => {
        let x = 120;
        return (
          <g key={row.title}>
            <Label x={14} y={row.y + 19} size={12.5} weight={600} anchor="start">
              {row.title}
            </Label>
            {row.packets.map((payload, index) => {
              const start = x;
              x += 3 * scale + payload * scale + 12;
              return (
                <g key={index}>
                  <rect x={start} y={row.y} width={3 * scale} height={28} rx={2} className="fill-fd-muted-foreground/60" />
                  <rect
                    x={start + 3 * scale}
                    y={row.y}
                    width={payload * scale}
                    height={28}
                    rx={2}
                    strokeWidth={1.25}
                    className="fill-primary/10 stroke-primary"
                  />
                  <Label x={start + 3 * scale + (payload * scale) / 2} y={row.y + 18.5} size={11.5} tone="primary">
                    {`${payload} bytes`}
                  </Label>
                </g>
              );
            })}
          </g>
        );
      })}
      <rect x={120} y={98} width={3 * scale} height={12} rx={2} className="fill-fd-muted-foreground/60" />
      <Label x={138} y={108} size={11} tone="muted" anchor="start">
        3-byte header
      </Label>
      <rect x={232} y={98} width={18} height={12} rx={2} strokeWidth={1.25} className="fill-primary/10 stroke-primary" />
      <Label x={256} y={108} size={11} tone="muted" anchor="start">
        payload, up to MTU minus 3
      </Label>
    </LearnFigure>
  );
}

export function ConnectionEventsDiagram() {
  const start = 104;
  const interval = 52;
  const events = [0, 1, 2, 3, 4, 5].map((index) => start + index * interval);
  const peripheralAnswers = [0, 3];

  return (
    <LearnFigure
      title="Connection events at the connection interval, with peripheral latency and the supervision timeout"
      caption="The central opens a connection event every connection interval. With a peripheral latency of 2, the peripheral may skip two events in a row to save power. If no packet gets through for the supervision timeout, both sides drop the connection."
      width={500}
      height={206}
    >
      <Label x={10} y={88} size={13} weight={600} anchor="start">
        Central
      </Label>
      <Label x={10} y={138} size={13} weight={600} anchor="start">
        Peripheral
      </Label>
      <line x1={start - 10} y1={94} x2={490} y2={94} strokeWidth={1} className="stroke-fd-border" />
      <line x1={start - 10} y1={144} x2={490} y2={144} strokeWidth={1} className="stroke-fd-border" />
      {events.map((x, index) => (
        <g key={x}>
          <rect x={x} y={72} width={10} height={22} rx={2} className="fill-primary" />
          {peripheralAnswers.includes(index) ? (
            <rect x={x + 2} y={122} width={10} height={22} rx={2} className="fill-fd-foreground" />
          ) : index < 3 ? (
            <rect x={x + 2} y={122} width={10} height={22} rx={2} strokeWidth={1.25} strokeDasharray="3 2" className="fill-none stroke-fd-muted-foreground" />
          ) : null}
        </g>
      ))}
      <path d={`M${events[0] + 5},62 V56 H${events[1] + 5} V62`} fill="none" strokeWidth={1.25} className="stroke-fd-muted-foreground" />
      <Label x={(events[0] + events[1]) / 2 + 5} y={48} size={11.5} tone="muted">
        connection interval
      </Label>
      <path d={`M${events[1] + 7},156 V162 H${events[2] + 7} V156`} fill="none" strokeWidth={1.25} className="stroke-fd-muted-foreground" />
      <Label x={(events[1] + events[2]) / 2 + 7} y={178} size={11.5} tone="muted">
        skipped: latency 2
      </Label>
      <path d={`M${events[3] + 12},40 V34 H486 V40`} fill="none" strokeWidth={1.25} className="stroke-primary" />
      <Label x={(events[3] + 486) / 2 + 6} y={26} size={11.5} tone="primary" weight={600}>
        supervision timeout
      </Label>
      <Label x={(events[4] + 486) / 2 + 6} y={190} size={11.5} tone="primary">
        no answer: connection lost
      </Label>
    </LearnFigure>
  );
}
