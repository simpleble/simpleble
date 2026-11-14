import type { ReactNode } from 'react';

import { cn } from '@/lib/cn';

type Tone = 'default' | 'primary' | 'muted';

const boxTones: Record<Tone, string> = {
  default: 'fill-fd-card stroke-fd-border',
  primary: 'fill-primary/10 stroke-primary',
  muted: 'fill-fd-muted stroke-fd-border',
};

const textTones: Record<Tone, string> = {
  default: 'fill-fd-foreground',
  primary: 'fill-primary',
  muted: 'fill-fd-muted-foreground',
};

const lineTones: Record<Tone, string> = {
  default: 'stroke-fd-foreground fill-fd-foreground',
  primary: 'stroke-primary fill-primary',
  muted: 'stroke-fd-muted-foreground fill-fd-muted-foreground',
};

export function LearnFigure({
  title,
  caption,
  width,
  height,
  maxWidth = 560,
  children,
}: {
  title: string;
  caption?: ReactNode;
  width: number;
  height: number;
  maxWidth?: number;
  children: ReactNode;
}) {
  return (
    <figure className="not-prose my-8">
      <svg
        viewBox={`0 0 ${width} ${height}`}
        role="img"
        aria-label={title}
        className="mx-auto block h-auto w-full font-sans"
        style={{ maxWidth }}
      >
        {children}
      </svg>
      {caption ? (
        <figcaption className="mx-auto mt-3 max-w-[560px] text-center text-sm text-fd-muted-foreground">
          {caption}
        </figcaption>
      ) : null}
    </figure>
  );
}

export function Box({
  x,
  y,
  w,
  h,
  tone = 'default',
  dashed = false,
  className,
}: {
  x: number;
  y: number;
  w: number;
  h: number;
  tone?: Tone;
  dashed?: boolean;
  className?: string;
}) {
  return (
    <rect
      x={x}
      y={y}
      width={w}
      height={h}
      rx={8}
      strokeWidth={1.5}
      strokeDasharray={dashed ? '5 4' : undefined}
      className={cn(boxTones[tone], className)}
    />
  );
}

export function Label({
  x,
  y,
  children,
  size = 14,
  weight = 400,
  tone = 'default',
  anchor = 'middle',
  mono = false,
}: {
  x: number;
  y: number;
  children: ReactNode;
  size?: number;
  weight?: number;
  tone?: Tone;
  anchor?: 'start' | 'middle' | 'end';
  mono?: boolean;
}) {
  return (
    <text
      x={x}
      y={y}
      fontSize={size}
      fontWeight={weight}
      textAnchor={anchor}
      className={cn(textTones[tone], mono && 'font-mono')}
    >
      {children}
    </text>
  );
}

function arrowHead(fromX: number, fromY: number, toX: number, toY: number) {
  const length = Math.hypot(toX - fromX, toY - fromY) || 1;
  const ux = (toX - fromX) / length;
  const uy = (toY - fromY) / length;
  const baseX = toX - ux * 9;
  const baseY = toY - uy * 9;
  return [
    `${toX},${toY}`,
    `${baseX - uy * 4.5},${baseY + ux * 4.5}`,
    `${baseX + uy * 4.5},${baseY - ux * 4.5}`,
  ].join(' ');
}

export function Arrow({
  x1,
  y1,
  x2,
  y2,
  tone = 'default',
  dashed = false,
  both = false,
}: {
  x1: number;
  y1: number;
  x2: number;
  y2: number;
  tone?: Tone;
  dashed?: boolean;
  both?: boolean;
}) {
  return (
    <g className={lineTones[tone]}>
      <line
        x1={x1}
        y1={y1}
        x2={x2}
        y2={y2}
        strokeWidth={1.75}
        strokeDasharray={dashed ? '6 5' : undefined}
      />
      <polygon points={arrowHead(x1, y1, x2, y2)} strokeWidth={0} />
      {both ? <polygon points={arrowHead(x2, y2, x1, y1)} strokeWidth={0} /> : null}
    </g>
  );
}
