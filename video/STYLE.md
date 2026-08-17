# SimpleAIBLE video style

Reference composition: `SimpleAIBLE` (`src/SimpleAIBLEVideo.tsx`). 1920×1080, 30 fps, ~22s.

## Structure

Four scenes:

| Scene | Role | Length |
|--------|------|--------|
| `HookScene` | Logo + one tagline | 90 frames (~3s) |
| `FeaturesScene` | Three capabilities, then a subtitle | 105 frames (~3.5s) |
| `DemoScene` (optional) | Screen recording | 210 frames (~7s) |
| `CTAScene` | Icon + install command + brand line | 225 frames (~7.5s) |

Between scenes: fade, 10 frames (`linearTiming`).

`TransitionSeries` overlaps the fade. Each sequence after the first is `sceneFrames + TRANSITION_DURATION`. Set `Composition` `durationInFrames` to the exact sum:

```text
90 + (105+10) + (210+10) + (225+10) = 660
```

Without `DemoScene`: omit that sequence and its fade, then recalculate. Recalculate whenever a scene length changes.

## Motion

**Background (every scene)**  
Ken Burns on `bg.png`: ~120% size, `objectFit: cover`, `overflow: hidden`. Scale and pan together.

- Background scale floor **1.08–1.15** while panning (never `1.0`).
- Scale easing: `Easing.inOut(Easing.quad)`. Pan easing: `Easing.inOut(Easing.sin)`.
- Overlay: radial darken, `rgba(0,0,0,0.1)` center → `0.3`–`0.4` edges.

**UI entrance**  
- Hook / CTA icon: spring, damping **8–15**, stiffness ~150.
- Feature rows: no bounce. Slide + fade, `Easing.out` (cubic/quad).
- Travel: 30–80px.

**Hold vs cut**  
End the scene shortly after the last animation. CTA branding stays on screen a couple of seconds. Cut frames if there is a pause between scenes.

## Typography and brand

- Sans: Inter via `--font-sans`
- Mono: JetBrains Mono via `--font-mono` (install command only)
- Sizes: hook `text-6xl`, features `text-7xl`, CTA command `text-4xl`/`text-5xl`, branding `text-3xl`
- Taglines: light weight. Semibold only on the stressed phrase
- White / white-80; `textShadow` (no card behind copy)
- Hook logo ~600px; CTA icon ~240px
- Logos: copy from `docs/public/` in the SimpleBLE repo (`simpleble-logo-color.svg` wordmark, `simpleble-icon-color.svg` icon, `SimpleBLE-logo-white.svg` if you need the white wordmark) into `video/public/`
- Background: `public/bg.png`

## Screen recordings (`DemoScene`) — optional

Skip this scene if the cut has no product recording. If you include it:

1. Trim the source clip. Skip dead stretches. This cut: `trimBefore` **16s**, playback **1x**.
2. Freeze 1 second (30 frames) on the last visible frame: `endAt={durationInFrames - 30}`.
3. Width ~1500px, centered, 16:9.
4. Light border, small radius, modest shadow. Same background as the type.
5. Clip camera: scale **1.0 → 1.05**, pan a few pixels. More pan requires scale above 1.0.
6. Delay zoom ~1s on open (`extrapolateLeft: "clamp"`).

### Two clips / crossfade

- Stack with `position: "absolute"` or `<Sequence>` in the same box.
- Crossfade: `Easing.inOut(Easing.sin)`.
- Keep the outgoing clip mounted through the fade (`<=` the overlap, not `<`).
- Subtract overlap from scene length (210 + 135 − 15 = 330).
- Do not multiply `startFrom` / `trimBefore` by `playbackRate`.

If the demo ends on an empty box, scene duration is longer than the media.

## CTA

Order: icon → terminal → typed command → brand line.

- Terminal: `bg-[#1a1a2e]/90`, traffic-light dots, green `$`, blinking cursor.
- Finish the typewriter with time left to read. Branding fades in and holds. If branding lands in the last few frames, extend the CTA and the root composition.
- Background scale ≥ ~1.08.

## New cut

1. Same fps, size, `bg.png`, fonts.
2. Replace copy, icons, and `public/demo.mov` if the cut has a demo.
3. If there is a recording: trim it; freeze the last second. Otherwise drop `DemoScene` and its fade.
4. Time each scene to its last animation; set `durationInFrames` to the TransitionSeries sum.
5. Check: no black bars, no black tail, no dead air between scenes. If there is a demo: no flash between parts.
6. Preview: `npm run dev`. Render: `npx remotion render`.
