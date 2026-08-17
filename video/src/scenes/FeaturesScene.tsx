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

const features = [
  { icon: "⚡", text: "MCP Server" },
  { icon: "⚡", text: "HTTP Server" },
  { icon: "⚡", text: "Skills" },
];

const FeatureItem: React.FC<{
  icon: string;
  text: string;
  delay: number;
}> = ({ icon, text, delay }) => {
  const frame = useCurrentFrame();

  const startFrame = delay;
  const endFrame = delay + 20;

  const opacity = interpolate(frame, [startFrame, endFrame], [0, 1], {
    extrapolateLeft: "clamp",
    extrapolateRight: "clamp",
    easing: Easing.out(Easing.ease),
  });

  const x = interpolate(frame, [startFrame, endFrame], [-60, 0], {
    extrapolateLeft: "clamp",
    extrapolateRight: "clamp",
    easing: Easing.out(Easing.cubic),
  });

  const scale = interpolate(frame, [startFrame, endFrame], [0.8, 1], {
    extrapolateLeft: "clamp",
    extrapolateRight: "clamp",
    easing: Easing.out(Easing.quad),
  });

  return (
    <div
      style={{
        opacity,
        transform: `translateX(${x}px) scale(${scale})`,
        textShadow: "0 2px 15px rgba(0,0,0,0.4)",
        fontFamily: "var(--font-sans)",
      }}
      className="flex items-center gap-6 text-white text-7xl font-medium"
    >
      <span>{icon}</span>
      <span>{text}</span>
    </div>
  );
};

export const FeaturesScene: React.FC = () => {
  const frame = useCurrentFrame();
  const { fps, durationInFrames } = useVideoConfig();

  // Dynamic background camera movement
  const bgScale = interpolate(frame, [0, durationInFrames], [1.05, 1.2], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.quad),
  });

  const bgPanX = interpolate(frame, [0, durationInFrames], [20, -20], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.sin),
  });

  const bgPanY = interpolate(frame, [0, durationInFrames], [-10, 15], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.sin),
  });

  // Subtitle appears after features
  const subtitleProgress = spring({
    frame,
    fps,
    config: { damping: 15, stiffness: 150 },
    delay: 60,
  });

  const subtitleOpacity = interpolate(subtitleProgress, [0, 1], [0, 1]);
  const subtitleY = interpolate(subtitleProgress, [0, 1], [30, 0]);

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
            "radial-gradient(ellipse at center, rgba(0,0,0,0.15) 0%, rgba(0,0,0,0.35) 100%)",
        }}
      />

      {/* Content */}
      <AbsoluteFill className="flex flex-col items-center justify-center gap-8">
        {/* Features list */}
        <div className="flex flex-col gap-6">
          {features.map((feature, index) => (
            <FeatureItem
              key={feature.text}
              icon={feature.icon}
              text={feature.text}
              delay={index * 15}
            />
          ))}
        </div>

        {/* Subtitle */}
        <div
          style={{
            opacity: subtitleOpacity,
            transform: `translateY(${subtitleY}px)`,
            marginTop: 40,
            textShadow: "0 2px 15px rgba(0,0,0,0.5)",
            fontFamily: "var(--font-sans)",
          }}
          className="text-white/80 text-5xl font-light"
        >
          Connect your agents to Bluetooth devices
        </div>
      </AbsoluteFill>
    </AbsoluteFill>
  );
};
