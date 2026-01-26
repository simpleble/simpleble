"use client";

import Image from "next/image";
import Link from "next/link";
import type { ReactElement } from "react";
import { useRef } from "react";
import { motion, useMotionTemplate, useMotionValue } from "framer-motion";

const AnimatedBackgroundImage = (): ReactElement => {
  return (
    <motion.div
      className="absolute"
      style={{
        top: "-5%",
        left: "-5%",
        width: "110%",
        height: "110%",
      }}
      initial={{
        x: "0%",
        y: "0%",
      }}
      animate={{
        x: ["-2%", "2%", "-2%"],
        y: ["-1.5%", "1.5%", "-1.5%"],
      }}
      transition={{
        duration: 50, // Ajusta este valor: más bajo = más rápido, más alto = más lento
        repeat: Infinity,
        ease: [0.25, 0.1, 0.25, 1], // Cubic bezier suave para rebote continuo sin paradas
        times: [0, 0.5, 1],
      }}
    >
      <Image
        src="/footer-bg.png"
        alt=""
        fill
        className="object-cover"
        style={{
          objectPosition: "left top",
        }}
        priority
      />
    </motion.div>
  );
};

export const DocsHomeHero = (): ReactElement => {
  const rootRef = useRef<HTMLDivElement | null>(null);
  const mouseX = useMotionValue(0);
  const mouseY = useMotionValue(0);
  const maskImage = useMotionTemplate`radial-gradient(280px at ${mouseX}px ${mouseY}px, white, transparent 72%)`;
  const maskStyle = { maskImage, WebkitMaskImage: maskImage } as const;

  return (
    <section
      ref={rootRef}
      className="group relative overflow-hidden rounded-[28px] border border-fd-border bg-fd-background"
      onMouseMove={(e) => {
        const rect = e.currentTarget.getBoundingClientRect();
        mouseX.set(e.clientX - rect.left);
        mouseY.set(e.clientY - rect.top);
      }}
    >
      <div className="pointer-events-none absolute inset-0">
        <div
          className="absolute inset-0 opacity-[0.9]"
          style={{
            background:
              "radial-gradient(900px 500px at 20% 10%, rgba(33,127,241,0.35) 0%, rgba(33,127,241,0) 65%), radial-gradient(700px 420px at 85% 0%, rgba(33,127,241,0.28) 0%, rgba(33,127,241,0) 70%)",
          }}
        />

        <div
          className="absolute inset-0 opacity-[0.35] dark:opacity-[0.25]"
          style={{
            backgroundImage:
              "linear-gradient(to right, rgba(23,23,23,0.06) 1px, transparent 1px), linear-gradient(to bottom, rgba(23,23,23,0.06) 1px, transparent 1px)",
            backgroundSize: "48px 48px",
          }}
        />

        <div className="absolute inset-0 mix-blend-soft-light opacity-[0.40] lg:opacity-[0.35] dark:opacity-[0.30] dark:lg:opacity-[0.25] overflow-hidden">
          <AnimatedBackgroundImage />
        </div>

        {/* Cursor illumination (mask follows cursor) */}
        <motion.div
          className="absolute inset-0 rounded-[28px] opacity-0 transition-opacity duration-300 group-hover:opacity-100"
          style={{
            background:
              "linear-gradient(to right, rgba(33,127,241,0.18), rgba(255,255,255,0.08))",
            ...maskStyle,
          }}
        />

        {/* Subtle core sheen */}
        <motion.div
          className="absolute inset-0 rounded-[28px] opacity-0 mix-blend-screen transition-opacity duration-300 group-hover:opacity-100"
          style={{
            background: "rgba(255,255,255,0.10)",
            ...maskStyle,
          }}
        />

        <div
          className="absolute -bottom-12 left-0 right-0 h-40 opacity-80 dark:opacity-90"
          style={{
            background:
              "linear-gradient(to top, rgba(33,127,241,0.20) 0%, rgba(33,127,241,0) 85%)",
          }}
        />
      </div>

      <div className="relative px-5 sm:px-8 lg:px-10 py-10 sm:py-14 lg:py-16">
        <div className="flex flex-col gap-8 sm:gap-10">
          <div className="flex flex-col gap-4">
            <p className="font-sora text-xs sm:text-sm tracking-[0.16em] uppercase text-fd-muted-foreground">
              SimpleBLE Docs
            </p>

            <h1 className="font-sora text-[30px] sm:text-[38px] lg:text-[48px] leading-[1.06] tracking-[-0.035em] text-fd-foreground">
              Bluetooth integration in minutes, not months.
            </h1>

            <p className="text-base sm:text-lg leading-relaxed text-fd-muted-foreground max-w-[70ch]">
              Learn the core API, pick the right bindings, and follow practical
              guides for building reliable Bluetooth experiences across
              platforms.
            </p>
          </div>

          <div className="flex flex-col sm:flex-row gap-3 sm:items-center">
            <Link
              href="/docs/simpleble/usage"
              className="inline-flex h-11 items-center justify-center rounded-2xl bg-primary px-5 text-sm font-sora font-semibold text-primary-foreground transition-colors hover:bg-primary/90"
            >
              Quickstart
            </Link>
            <Link
              href="/docs/simpleble/api"
              className="inline-flex h-11 items-center justify-center rounded-2xl border border-fd-border bg-fd-background/70 px-5 text-sm font-sora font-semibold text-fd-foreground backdrop-blur transition-colors hover:bg-fd-muted"
            >
              Explore API
            </Link>
            <Link
              href="/docs"
              className="inline-flex h-11 items-center justify-center rounded-2xl px-4 text-sm font-sora font-semibold text-primary transition-colors hover:bg-primary/10"
            >
              Browse all docs →
            </Link>
          </div>
        </div>
      </div>
    </section>
  );
};
