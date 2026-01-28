import type { ReactNode } from 'react';
import type { ImageResponse } from 'next/og';

type ImageResponseOptions = NonNullable<ConstructorParameters<typeof ImageResponse>[1]>;

export interface GenerateProps {
  title: ReactNode;
  description?: ReactNode;
  logoUrl?: string;
}

export async function getImageResponseOptions(): Promise<ImageResponseOptions> {
  return {
    width: 1200,
    height: 630,
  };
}

export function generate({ title, description, logoUrl }: GenerateProps) {
  const primaryColor = "#217ff1";
  const backgroundColor = "#ffffff";
  const foregroundColor = "#171717";
  const mutedForegroundColor = "#6b6b6b";

  return (
    <div
      style={{
        height: "100%",
        width: "100%",
        display: "flex",
        flexDirection: "column",
        backgroundColor: backgroundColor,
        padding: "60px 80px",
        position: "relative",
        overflow: "hidden",
      }}
    >
      {/* Background Gradients */}
      <div
        style={{
          position: "absolute",
          top: 0,
          left: 0,
          right: 0,
          bottom: 0,
          display: "flex",
          opacity: 0.9,
          background: "radial-gradient(900px 500px at 20% 10%, rgba(33,127,241,0.35) 0%, rgba(33,127,241,0) 65%), radial-gradient(700px 420px at 85% 0%, rgba(33,127,241,0.28) 0%, rgba(33,127,241,0) 70%)",
        }}
      />

      {/* Grid Pattern */}
      <div
        style={{
          position: "absolute",
          top: 0,
          left: 0,
          right: 0,
          bottom: 0,
          display: "flex",
          opacity: 0.35,
          backgroundImage: "linear-gradient(to right, rgba(23,23,23,0.06) 1px, transparent 1px), linear-gradient(to bottom, rgba(23,23,23,0.06) 1px, transparent 1px)",
          backgroundSize: "48px 48px",
        }}
      />

      {/* Bottom Accent Gradient */}
      <div
        style={{
          position: "absolute",
          bottom: "-48px",
          left: 0,
          right: 0,
          height: "160px",
          display: "flex",
          opacity: 0.8,
          background: "linear-gradient(to top, rgba(33,127,241,0.20) 0%, rgba(33,127,241,0) 85%)",
        }}
      />

      {/* Content Container */}
      <div
        style={{
          display: "flex",
          flexDirection: "column",
          height: "100%",
          width: "100%",
          position: "relative",
        }}
      >
        {/* Header Tag */}
        <p
          style={{
            fontFamily: "sans-serif",
            fontSize: "14px",
            fontWeight: 600,
            textTransform: "uppercase",
            letterSpacing: "0.16em",
            color: mutedForegroundColor,
            margin: "0 0 40px 0",
          }}
        >
          SimpleBLE Docs
        </p>

        <div style={{ display: "flex", flexDirection: "column", gap: "24px", flex: 1 }}>
          {/* Title */}
          <h1
            style={{
              fontSize: "72px",
              fontWeight: 700,
              color: foregroundColor,
              lineHeight: 1.06,
              margin: 0,
              letterSpacing: "-0.035em",
              fontFamily: "sans-serif",
            }}
          >
            {title}
          </h1>

          {/* Description */}
          <p
            style={{
              fontSize: "32px",
              color: mutedForegroundColor,
              lineHeight: 1.4,
              maxWidth: "900px",
              margin: 0,
              fontFamily: "sans-serif",
            }}
          >
            {description}
          </p>
        </div>

        {/* Footer */}
        <div
          style={{
            display: "flex",
            alignItems: "center",
            justifyContent: "space-between",
            width: "100%",
            marginTop: "auto",
          }}
        >
          <div style={{ display: "flex", alignItems: "center" }}>
            {logoUrl && (
              <img
                src={logoUrl}
                alt="SimpleBLE"
                style={{
                  height: "45px",
                  width: "auto",
                }}
              />
            )}
          </div>
          <div style={{ display: "flex", alignItems: "center", gap: "10px" }}>
            <div
              style={{
                width: "60px",
                height: "3px",
                backgroundColor: primaryColor,
              }}
            />
          </div>
        </div>
      </div>
    </div>
  );
}
