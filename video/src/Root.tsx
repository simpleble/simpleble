import "./index.css";
import { Composition } from "remotion";
import { SimpleAIBLEVideo } from "./SimpleAIBLEVideo";

export const RemotionRoot: React.FC = () => {
  return (
    <>
      <Composition
        id="SimpleAIBLE"
        component={SimpleAIBLEVideo}
        durationInFrames={660}
        fps={30}
        width={1920}
        height={1080}
      />
    </>
  );
};
