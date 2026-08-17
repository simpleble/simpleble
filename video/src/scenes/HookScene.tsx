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

export const HookScene: React.FC = () => {
  const frame = useCurrentFrame();
  const { fps, durationInFrames } = useVideoConfig();

  // Dynamic background camera movement
  const bgScale = interpolate(frame, [0, durationInFrames], [1.0, 1.15], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.quad),
  });

  const bgPanX = interpolate(frame, [0, durationInFrames], [0, -30], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.sin),
  });

  const bgPanY = interpolate(frame, [0, durationInFrames], [0, -20], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.sin),
  });

  // Logo entrance with spring
  const logoProgress = spring({
    frame,
    fps,
    config: { damping: 15, stiffness: 150 },
    delay: 5,
  });

  const logoScale = interpolate(logoProgress, [0, 1], [0.6, 1]);
  const logoOpacity = interpolate(logoProgress, [0, 1], [0, 1]);

  // Text fade in
  const textOpacity = interpolate(frame, [20, 45], [0, 1], {
    extrapolateLeft: "clamp",
    extrapolateRight: "clamp",
  });

  const textY = interpolate(frame, [20, 45], [30, 0], {
    extrapolateLeft: "clamp",
    extrapolateRight: "clamp",
    easing: Easing.out(Easing.back(1.5)),
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

      {/* Subtle dark overlay for better text contrast */}
      <AbsoluteFill
        style={{
          background:
            "radial-gradient(ellipse at center, rgba(0,0,0,0.1) 0%, rgba(0,0,0,0.3) 100%)",
        }}
      />

      {/* Content */}
      <AbsoluteFill className="flex flex-col items-center justify-center">
        {/* Logo */}
        <Img
          src={staticFile("simpleble-logo-color.svg")}
          style={{
            width: 600,
            opacity: logoOpacity,
            transform: `scale(${logoScale})`,
            filter: "drop-shadow(0 4px 20px rgba(0,0,0,0.3))",
          }}
        />

        {/* Tagline */}
        <div
          style={{
            opacity: textOpacity,
            transform: `translateY(${textY}px)`,
            marginTop: 60,
            textShadow: "0 2px 20px rgba(0,0,0,0.5)",
            fontFamily: "var(--font-sans)",
          }}
          className="text-white text-6xl font-light tracking-wide text-center max-w-5xl"
        >
          Your AI agents just got{" "}
          <span className="font-semibold">hardware superpowers</span>
        </div>
      </AbsoluteFill>
    </AbsoluteFill>
  );
};
