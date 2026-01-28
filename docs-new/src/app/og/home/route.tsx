import { ImageResponse } from "next/og";

export const revalidate = false;

export async function GET(_req: Request) {
  // Aesthetic constants based on global.css and DocsHomeHero
  const primaryColor = "#217ff1";
  const backgroundColor = "#ffffff";
  const foregroundColor = "#171717";
  const mutedForegroundColor = "#6b6b6b";

  const title = "SimpleBLE Docs";
  const description = "The fully supported, cross-platform library designed to get your embedded product to market without the headaches.";

  // Use the dark logo for light background (simpleBLE-logo-dark.png)
  const logoUrl = new URL("/simpleBLE-logo-dark.png", _req.url).toString();
  const bgUrl = new URL("/footer-bg.png", _req.url).toString();

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
        {/* Background Image (Same as Hero) */}
        <div
          style={{
            position: "absolute",
            top: "-10%",
            left: "-10%",
            width: "120%",
            height: "120%",
            display: "flex",
            opacity: 0.15,
            backgroundImage: `url(${bgUrl})`,
            backgroundSize: "cover",
            backgroundPosition: "left top",
          }}
        />

        {/* Background Gradients (Simplified version of DocsHomeHero) */}
        <div
          style={{
            position: "absolute",
            top: 0,
            left: 0,
            right: 0,
            bottom: 0,
            display: "flex",
            opacity: 0.6,
            background: `radial-gradient(900px 500px at 20% 10%, rgba(33,127,241,0.35) 0%, rgba(33,127,241,0) 65%), radial-gradient(700px 420px at 85% 0%, rgba(33,127,241,0.28) 0%, rgba(33,127,241,0) 70%)`,
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
            opacity: 0.08,
            backgroundImage: `linear-gradient(to right, rgba(23,23,23,0.1) 1px, transparent 1px), linear-gradient(to bottom, rgba(23,23,23,0.1) 1px, transparent 1px)`,
            backgroundSize: "48px 48px",
          }}
        />

        {/* Bottom sheen gradient */}
        <div
          style={{
            position: "absolute",
            bottom: 0,
            left: 0,
            right: 0,
            height: "200px",
            display: "flex",
            background: "linear-gradient(to top, rgba(33,127,241,0.15) 0%, rgba(33,127,241,0) 100%)",
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
