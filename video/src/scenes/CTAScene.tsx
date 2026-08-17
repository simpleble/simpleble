import {
  AbsoluteFill,
  Img,
  staticFile,
  useCurrentFrame,
  useVideoConfig,
  interpolate,
  spring,
  Easing,
} from "remotion";

export const CTAScene: React.FC = () => {
  const frame = useCurrentFrame();
  const { fps, durationInFrames } = useVideoConfig();

  // Dynamic background camera movement
  const bgScale = interpolate(frame, [0, durationInFrames], [1.15, 1.08], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.quad),
  });

  const bgPanX = interpolate(frame, [0, durationInFrames], [-15, 15], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.sin),
  });

  const bgPanY = interpolate(frame, [0, durationInFrames], [10, -10], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.sin),
  });

  // Icon bounce entrance
  const iconProgress = spring({
    frame,
    fps,
    config: { damping: 8, stiffness: 150 },
    delay: 0,
  });

  const iconScale = interpolate(iconProgress, [0, 1], [0, 1.05]);
  const iconOpacity = interpolate(iconProgress, [0, 1], [0, 1]);

  // Command text typewriter effect (faster)
  const command = "pip install simpleaible";
  const charsToShow = Math.floor(
    interpolate(frame, [25, 70], [0, command.length], {
      extrapolateLeft: "clamp",
      extrapolateRight: "clamp",
    }),
  );
  const displayedCommand = command.slice(0, charsToShow);

  // Cursor blink (faster)
  const cursorOpacity = Math.floor(frame / 12) % 2 === 0 ? 1 : 0;

  // Terminal box fade in (faster)
  const terminalOpacity = interpolate(frame, [15, 30], [0, 1], {
    extrapolateLeft: "clamp",
    extrapolateRight: "clamp",
  });

  // Branding text fade in (earlier and faster)
  const brandingOpacity = interpolate(frame, [75, 95], [0, 1], {
    extrapolateLeft: "clamp",
    extrapolateRight: "clamp",
  });

  return (
    <AbsoluteFill style={{ overflow: "hidden" }}>
      {/* Background with dynamic movement */}
      <AbsoluteFill>
        <Img
          src={staticFile("bg.png")}
          style={{
            width: "120%",
            height: "120%",
            objectFit: "cover",
            transform: `scale(${bgScale}) translate(${bgPanX}px, ${bgPanY}px)`,
          }}
        />
      </AbsoluteFill>

      {/* Subtle dark overlay */}
      <AbsoluteFill
        style={{
          background:
            "radial-gradient(ellipse at center, rgba(0,0,0,0.1) 0%, rgba(0,0,0,0.3) 100%)",
        }}
      />

      {/* Content */}
      <AbsoluteFill className="flex flex-col items-center justify-center gap-10">
        {/* Icon with bounce */}
        <Img
          src={staticFile("simpleble-icon-color.svg")}
          style={{
            width: 240,
            opacity: iconOpacity,
            transform: `scale(${iconScale})`,
            filter: "drop-shadow(0 4px 30px rgba(0,0,0,0.3))",
          }}
        />

        {/* Terminal command box */}
        <div
          style={{
            opacity: terminalOpacity,
            boxShadow: "0 25px 60px rgba(0, 0, 0, 0.4)",
          }}
          className="bg-[#1a1a2e]/90 backdrop-blur-sm border border-white/10 rounded-xl px-12 py-8"
        >
          {/* Terminal header dots */}
          <div className="flex gap-3 mb-6">
            <div className="w-4 h-4 rounded-full bg-[#ff5f56]" />
            <div className="w-4 h-4 rounded-full bg-[#ffbd2e]" />
            <div className="w-4 h-4 rounded-full bg-[#27ca40]" />
          </div>

          {/* Command */}
          <div 
            className="text-4xl"
            style={{ fontFamily: "var(--font-mono)" }}
          >
            <span className="text-[#27ca40]">$</span>{" "}
            <span className="text-white">{displayedCommand}</span>
            <span
              style={{ opacity: cursorOpacity }}
              className="text-white ml-1"
            >
              |
            </span>
          </div>
        </div>

        {/* Branding text */}
        <div
          style={{
            opacity: brandingOpacity,
            textShadow: "0 2px 15px rgba(0,0,0,0.5)",
            fontFamily: "var(--font-sans)",
          }}
          className="text-white/90 text-3xl font-light mt-8"
        >
          SimpleAIBLE. Your complete toolkit for Bluetooth development.
        </div>
      </AbsoluteFill>
    </AbsoluteFill>
  );
};
