import { ImageResponse } from "next/og";

export const revalidate = false;

export async function GET(_req: Request) {
  // Aesthetic constants based on global.css and DocsHomeHero
  const primaryColor = "#217ff1";
  const backgroundColor = "#ffffff";
  const foregroundColor = "#171717";
  const mutedForegroundColor = "#6b6b6b";

  const title = "SimpleBLE Docs";
  const description = "Learn the core API, pick the right bindings, and follow practical guides for building reliable Bluetooth experiences across platforms.";

  // Use the dark logo for light background (simpleBLE-logo-dark.png)
  const logoUrl = new URL("/simpleBLE-logo-dark.png", _req.url).toString();

  return new ImageResponse(
    (
      <div
        style={{
          height: "100%",
          width: "100%",
          display: "flex",
          flexDirection: "column",
          backgroundColor: backgroundColor,
          padding: "80px",
          position: "relative",
          fontFamily: "sans-serif",
        }}
      >
        {/* Background Gradients (Simplified version of DocsHomeHero) */}
        <div
          style={{
            position: "absolute",
            top: 0,
            left: 0,
            right: 0,
            bottom: 0,
            display: "flex",
            opacity: 0.4,
            background: `radial-gradient(900px 500px at 20% 10%, rgba(33,127,241,0.25) 0%, rgba(33,127,241,0) 65%), radial-gradient(700px 420px at 85% 0%, rgba(33,127,241,0.2) 0%, rgba(33,127,241,0) 70%)`,
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
            opacity: 0.05,
            backgroundImage: `linear-gradient(to right, ${foregroundColor} 1px, transparent 1px), linear-gradient(to bottom, ${foregroundColor} 1px, transparent 1px)`,
            backgroundSize: "48px 48px",
          }}
        />

        {/* Content Container */}
        <div
          style={{
            display: "flex",
            flexDirection: "column",
            justifyContent: "space-between",
            height: "100%",
            position: "relative",
            zIndex: 10,
          }}
        >
          <div style={{ display: "flex", flexDirection: "column", gap: "20px" }}>
            {/* Title */}
            <h1
              style={{
                fontSize: "72px",
                fontWeight: "bold",
                color: foregroundColor,
                lineHeight: 1.1,
                margin: "0 0 20px 0",
                letterSpacing: "-0.04em",
              }}
            >
              {title}
            </h1>

            {/* Description / First Paragraph */}
            <p
              style={{
                fontSize: "32px",
                color: mutedForegroundColor,
                lineHeight: 1.4,
                maxWidth: "900px",
                margin: 0,
              }}
            >
              {description}
            </p>
          </div>

          {/* Footer info - Logo moved here */}
          <div
            style={{
              display: "flex",
              alignItems: "center",
              justifyContent: "space-between",
              width: "100%",
            }}
          >
            <div style={{ display: "flex", alignItems: "center" }}>
              <img
                src={logoUrl}
                alt="SimpleBLE"
                style={{
                  height: "45px",
                  width: "auto",
                }}
              />
            </div>
            <div style={{ display: "flex", alignItems: "center", gap: "10px" }}>
              <div
                style={{
                  width: "40px",
                  height: "2px",
                  backgroundColor: primaryColor,
                }}
              />
            </div>
          </div>
        </div>
      </div>
    ),
    {
      width: 1200,
      height: 630,
    }
  );
}

export function generateStaticParams() {
  // Return empty array to indicate this route should be statically generated
  return [];
}
