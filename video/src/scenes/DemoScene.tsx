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
import { Video } from "@remotion/media";

// Demo video settings
// Start at 16 seconds, play at normal speed
const DEMO_START_SECONDS = 16;
const DEMO_PLAYBACK_RATE = 1;

export const DemoScene: React.FC = () => {
  const frame = useCurrentFrame();
  const { fps, durationInFrames } = useVideoConfig();

  // Video container entrance from bottom with spring
  const containerProgress = spring({
    frame,
    fps,
    config: { damping: 15, stiffness: 150 },
    delay: 0,
  });

  const containerY = interpolate(containerProgress, [0, 1], [100, 0]);
  const containerOpacity = interpolate(containerProgress, [0, 1], [0, 1]);

  // Dynamic camera movement on the video
  // Zoom: starts at 1.0, subtle zoom in over the scene
  const videoScale = interpolate(frame, [0, durationInFrames], [1.0, 1.05], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.quad),
  });

  // Pan: minimal horizontal drift to avoid borders
  const videoPanX = interpolate(frame, [0, durationInFrames], [-5, 10], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.sin),
  });

  // Pan: minimal vertical drift to avoid borders
  const videoPanY = interpolate(frame, [0, durationInFrames], [0, -5], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.sin),
  });

  // Dynamic background camera movement
  const bgScale = interpolate(frame, [0, durationInFrames], [1.05, 1.15], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.quad),
  });

  const bgPanX = interpolate(frame, [0, durationInFrames], [15, -25], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.sin),
  });

  const bgPanY = interpolate(frame, [0, durationInFrames], [-5, 20], {
    extrapolateRight: "clamp",
    easing: Easing.inOut(Easing.sin),
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
            "radial-gradient(ellipse at center, rgba(0,0,0,0.2) 0%, rgba(0,0,0,0.4) 100%)",
        }}
      />

      {/* Content */}
      <AbsoluteFill className="flex flex-col items-center justify-center">
        {/* Video with frame/shadow */}
        <div
          style={{
            opacity: containerOpacity,
            transform: `translateY(${containerY}px)`,
          }}
          className="relative"
        >
          {/* Glow effect behind video */}
          <div
            className="absolute inset-0 blur-3xl"
            style={{
              background: "rgba(255, 255, 255, 0.25)",
              transform: "scale(0.85)",
            }}
          />

          {/* Video container with overflow hidden for pan/zoom effect */}
          <div
            style={{
              width: 1500,
              borderRadius: 20,
              overflow: "hidden",
              boxShadow: "0 30px 100px rgba(0, 0, 0, 0.6)",
            }}
          >
            {/* Demo Video with dynamic camera movement */}
            <Video
              src={staticFile("demo.mov")}
              trimBefore={DEMO_START_SECONDS * fps}
              endAt={durationInFrames - 30}
              playbackRate={DEMO_PLAYBACK_RATE}
              muted
              style={{
                width: "100%",
                transform: `scale(${videoScale}) translate(${videoPanX}px, ${videoPanY}px)`,
              }}
            />
          </div>
        </div>
      </AbsoluteFill>
    </AbsoluteFill>
  );
};
