import { AbsoluteFill } from "remotion";
import { TransitionSeries, linearTiming } from "@remotion/transitions";
import { fade } from "@remotion/transitions/fade";
import { HookScene } from "./scenes/HookScene";
import { FeaturesScene } from "./scenes/FeaturesScene";
import { DemoScene } from "./scenes/DemoScene";
import { CTAScene } from "./scenes/CTAScene";

// 21 seconds @ 30fps = 630 frames total
// Scene 1: Hook (0-3s) = 90 frames
// Scene 2: Features (3-6.5s) = 105 frames
// Scene 3: Demo (6.5-13.5s) = 210 frames
// Scene 4: CTA (13.5-21s) = 225 frames

const TRANSITION_DURATION = 10;

export const SimpleAIBLEVideo: React.FC = () => {
  return (
    <AbsoluteFill className="bg-[#0a0a1a]">
      <TransitionSeries>
        <TransitionSeries.Sequence durationInFrames={90}>
          <HookScene />
        </TransitionSeries.Sequence>

        <TransitionSeries.Transition
          presentation={fade()}
          timing={linearTiming({ durationInFrames: TRANSITION_DURATION })}
        />

        <TransitionSeries.Sequence durationInFrames={105 + TRANSITION_DURATION}>
          <FeaturesScene />
        </TransitionSeries.Sequence>

        <TransitionSeries.Transition
          presentation={fade()}
          timing={linearTiming({ durationInFrames: TRANSITION_DURATION })}
        />

        <TransitionSeries.Sequence durationInFrames={210 + TRANSITION_DURATION}>
          <DemoScene />
        </TransitionSeries.Sequence>

        <TransitionSeries.Transition
          presentation={fade()}
          timing={linearTiming({ durationInFrames: TRANSITION_DURATION })}
        />

        <TransitionSeries.Sequence durationInFrames={225 + TRANSITION_DURATION}>
          <CTAScene />
        </TransitionSeries.Sequence>
      </TransitionSeries>
    </AbsoluteFill>
  );
};
