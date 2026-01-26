"use client";

import type { ReactNode } from "react";
import { motion, useMotionTemplate, useMotionValue } from "framer-motion";

type HoverEffectWrapperProps = {
  children: ReactNode;
  className?: string;
  radius?: number;
  variant?: "default" | "button";
};

const variants = {
  default: {
    gradient:
      "linear-gradient(to right, rgba(33,127,241,0.18), rgba(255,255,255,0.08))",
    sheen: "rgba(255,255,255,0.08)",
  },
  button: {
    gradient:
      "linear-gradient(to right, rgba(33,127,241,0.35), rgba(33,127,241,0.20))",
    sheen: "rgba(255,255,255,0.15)",
  },
};

export const HoverEffectWrapper = ({
  children,
  className = "",
  radius = 120,
  variant = "default",
}: HoverEffectWrapperProps) => {
  const mouseX = useMotionValue(0);
  const mouseY = useMotionValue(0);

  const maskImage = useMotionTemplate`radial-gradient(${radius}px at ${mouseX}px ${mouseY}px, white, transparent 70%)`;
  const maskStyle = { maskImage, WebkitMaskImage: maskImage } as const;

  const colors = variants[variant];

  return (
    <div
      className={`group relative ${className}`}
      onMouseMove={(e) => {
        const rect = e.currentTarget.getBoundingClientRect();
        mouseX.set(e.clientX - rect.left);
        mouseY.set(e.clientY - rect.top);
      }}
    >
      {/* Cursor illumination (mask follows cursor) */}
      <motion.div
        aria-hidden="true"
        className="pointer-events-none absolute inset-0 rounded-[inherit] opacity-0 transition-opacity duration-300 group-hover:opacity-100"
        style={{
          background: colors.gradient,
          ...maskStyle,
        }}
      />

      {/* Subtle core sheen */}
      <motion.div
        aria-hidden="true"
        className="pointer-events-none absolute inset-0 rounded-[inherit] opacity-0 mix-blend-screen transition-opacity duration-300 group-hover:opacity-100"
        style={{
          background: colors.sheen,
          ...maskStyle,
        }}
      />

      {children}
    </div>
  );
};
