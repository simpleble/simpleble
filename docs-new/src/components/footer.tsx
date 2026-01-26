import Image from 'next/image'
import Link from 'next/link'

type FooterLinkProps = Readonly<{
  href: string
  children: React.ReactNode
}>

const FooterLink = ({
  href,
  children,
}: FooterLinkProps): React.ReactElement => (
  <Link
    href={href}
    className="font-sora text-base 4k:text-lg text-white/80 hover:text-white transition-colors leading-[1.5]"
  >
    {children}
  </Link>
)

type SocialIconProps = Readonly<{
  href: string
  label: string
  icon: 'facebook' | 'twitter' | 'linkedin'
}>

const SocialIcon = ({
  href,
  label,
  icon,
}: SocialIconProps): React.ReactElement => {
  const icons = {
    facebook: (
      <svg className="w-[18px] h-[18px] 4k:w-[22px] 4k:h-[22px]" viewBox="0 0 18 18" fill="none">
        <rect width="18" height="18" rx="9" fill="white" />
        <path
          d="M9.94 9.5h1.76l.28-2h-2.04V6.75c0-.69.34-1.35 1.41-1.35h.87V3.7s-.79-.13-1.54-.13c-1.57 0-2.6.95-2.6 2.68v1.25H6.5v2h1.58v4.83c.32.05.64.08.97.08.33 0 .65-.03.95-.08V9.5z"
          fill="#217FF1"
        />
      </svg>
    ),
    twitter: (
      <svg className="w-[18px] h-[18px] 4k:w-[22px] 4k:h-[22px]" viewBox="0 0 18 18" fill="none">
        <rect width="18" height="18" rx="9" fill="white" />
        <path
          d="M13.183 4.188h1.242l-2.713 3.103 3.194 4.221h-2.498l-1.958-2.563-2.24 2.563H6.968l2.901-3.315-3.058-4.009h2.561l1.769 2.338 2.042-2.338zm-.436 6.579h.688L8.532 5.114h-.738l4.953 5.653z"
          fill="#217FF1"
        />
      </svg>
    ),
    linkedin: (
      <svg className="w-[18px] h-[18px] 4k:w-[22px] 4k:h-[22px]" viewBox="0 0 18 18" fill="none">
        <rect width="18" height="18" rx="9" fill="white" />
        <path
          d="M6.5 7.5h1.5v4.5H6.5V7.5zM7.25 6.75c-.414 0-.75-.336-.75-.75s.336-.75.75-.75.75.336.75.75-.336.75-.75.75zM12 12h-1.5V9.75c0-.563-.012-1.285-.783-1.285-.784 0-.904.612-.904 1.244V12H7.5V7.5h1.44v.616h.02c.2-.379.69-.783 1.42-.783 1.519 0 1.8 1 1.8 2.3V12h-.18z"
          fill="#217FF1"
        />
      </svg>
    ),
  }

  return (
    <Link
      href={href}
      aria-label={label}
      className="w-9 h-9 4k:w-11 4k:h-11 rounded-full border border-white/12 flex items-center justify-center hover:bg-white/10 transition-colors"
    >
      {icons[icon]}
    </Link>
  )
}

export const Footer = (): React.ReactElement => {
  return (
    <footer className="relative bg-[#217ff1] overflow-hidden">
      {/* Background image with soft-light blend */}
      <div className="absolute inset-0 w-full h-full flex items-center justify-center mix-blend-soft-light">
        <div className="relative w-full h-full">
          <Image
            src="/footer-bg.png"
            alt=""
            fill
            className="object-cover rotate-180 scale-y-[-1]"
            priority
          />
        </div>
      </div>

      {/* Content */}
      <div className="relative z-10 px-4 md:px-16 4k:px-24 py-20 4k:py-28">
        <div className="max-w-[1312px] 4k:max-w-[1800px] mx-auto flex flex-col gap-20 lg:gap-[140px] 4k:gap-[180px]">
          {/* Top CTA Section */}
          <div className="max-w-[768px] 4k:max-w-[900px]">
            <div className="flex flex-col gap-8 4k:gap-10">
              <div className="flex flex-col gap-6 4k:gap-8">
                <h2 className="font-sora text-[40px] md:text-[56px] 4k:text-[72px] text-white leading-[1.1] tracking-[-3.36px] 4k:tracking-[-4.32px]">
                  Stop fighting with Bluetooth.
                  <br />
                  Start shipping your product.
                </h2>
                <p className="font-sora text-lg 4k:text-xl text-white max-w-[449px] 4k:max-w-[550px] leading-[1.5]">
                  Join the developers who switched to SimpleBLE and got their
                  connection working in minutes.
                </p>
              </div>

              <div className="flex flex-col sm:flex-row gap-3 4k:gap-4">
                <Link
                  href="https://github.com/OpenBluetoothToolbox/SimpleBLE"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="px-5 py-4 4k:px-6 4k:py-5 bg-white rounded-2xl text-[#217ff1] font-sora font-semibold md:text-lg 4k:text-xl tracking-[-0.36px] hover:bg-white/90 transition-colors flex items-center justify-center gap-2.5 leading-[1.2]"
                >
                  Start building on GitHub
                  <svg
                    className="w-[30px] h-[30px] 4k:w-[36px] 4k:h-[36px] rotate-180 scale-y-[-1]"
                    viewBox="0 0 24 24"
                    fill="currentColor"
                  >
                    <path
                      fillRule="evenodd"
                      d="M12 2C6.477 2 2 6.484 2 12.017c0 4.425 2.865 8.18 6.839 9.504.5.092.682-.217.682-.483 0-.237-.008-.868-.013-1.703-2.782.605-3.369-1.343-3.369-1.343-.454-1.158-1.11-1.466-1.11-1.466-.908-.62.069-.608.069-.608 1.003.07 1.531 1.032 1.531 1.032.892 1.53 2.341 1.088 2.91.832.092-.647.35-1.088.636-1.338-2.22-.253-4.555-1.113-4.555-4.951 0-1.093.39-1.988 1.029-2.688-.103-.253-.446-1.272.098-2.65 0 0 .84-.27 2.75 1.026A9.564 9.564 0 0112 6.844c.85.004 1.705.115 2.504.337 1.909-1.296 2.747-1.027 2.747-1.027.546 1.379.202 2.398.1 2.651.64.7 1.028 1.595 1.028 2.688 0 3.848-2.339 4.695-4.566 4.943.359.309.678.92.678 1.855 0 1.338-.012 2.419-.012 2.747 0 .268.18.58.688.482A10.019 10.019 0 0022 12.017C22 6.484 17.522 2 12 2z"
                      clipRule="evenodd"
                    />
                  </svg>
                </Link>
                <Link
                  href="/contact"
                  className="px-5 py-4 4k:px-6 4k:py-5 border border-white/32 rounded-2xl text-white font-sora font-semibold md:text-lg 4k:text-xl tracking-[-0.36px] hover:bg-white/10 transition-colors text-center h-[62px] 4k:h-[72px] flex items-center justify-center leading-[1.2]"
                >
                  Contact us
                </Link>
              </div>
            </div>
          </div>

          {/* Bottom Section */}
          <div className="flex flex-col gap-[100px] 4k:gap-[120px]">
            {/* Links Section */}
            <div className="flex flex-col lg:flex-row justify-between gap-12 lg:gap-8 4k:gap-12">
              {/* Logo and description */}
              <div className="flex flex-col gap-5 4k:gap-6 max-w-[511px] 4k:max-w-[600px]">
                <Image
                  src="/simpleBLE-logo.png"
                  alt="SimpleBLE"
                  width={152}
                  height={50}
                  className="h-[50px] w-[152px] 4k:h-[60px] 4k:w-[182px]"
                />
                <p className="font-sora text-base 4k:text-lg text-white/80 leading-[1.5]">
                  SimpleBLE is the fully supported, cross-platform library (C++,
                  Python, Java,...) designed to get your embedded product to
                  market without the headaches.
                </p>
              </div>

              {/* Navigation Links */}
              <div className="flex gap-10 4k:gap-14">
                {/* Get Started */}
                <div className="flex flex-col gap-5 4k:gap-6 w-[217px] 4k:w-[260px]">
                  <p className="font-sora font-semibold text-base 4k:text-lg text-white tracking-[-0.32px] leading-none">
                    Get Started
                  </p>
                  <div className="flex flex-col gap-3 4k:gap-4">
                    <FooterLink href="/contact">Get in Contact</FooterLink>
                    <FooterLink href="/resources">Resources</FooterLink>
                    <FooterLink href="/consulting">Consulting</FooterLink>
                    <FooterLink href="/hardware">
                      Hardware (coming soon)
                    </FooterLink>
                  </div>
                </div>

                {/* Legal */}
                <div className="flex flex-col gap-5 4k:gap-6 w-[217px] 4k:w-[260px]">
                  <p className="font-sora text-base 4k:text-lg text-white tracking-[-0.32px] leading-none">
                    Legal
                  </p>
                  <div className="flex flex-col gap-3 4k:gap-4">
                    <FooterLink href="/terms">Terms of Service</FooterLink>
                    <FooterLink href="/privacy">Privacy Policy</FooterLink>
                    <FooterLink href="/cookies">Cookie Policy</FooterLink>
                  </div>
                </div>
              </div>
            </div>

            {/* Copyright and Social */}
            <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-9 4k:gap-12">
              <p className="font-sora text-sm 4k:text-base text-white/80 leading-[1.5]">
                © Copyright 2025 SimpleBLE
              </p>

              <div className="flex items-center gap-2.5 4k:gap-3">
                <SocialIcon
                  href="https://facebook.com"
                  label="Facebook"
                  icon="facebook"
                />
                <SocialIcon
                  href="https://x.com"
                  label="X (Twitter)"
                  icon="twitter"
                />
                <SocialIcon
                  href="https://linkedin.com"
                  label="LinkedIn"
                  icon="linkedin"
                />
              </div>
            </div>
          </div>
        </div>
      </div>
    </footer>
  )
}
