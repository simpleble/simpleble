'use client'

import { useState } from 'react'
import Image from 'next/image'
import Link from 'next/link'

type NavbarProps = Readonly<{
  variant?: 'dark' | 'light'
  isDocs?: boolean
}>

export const Navbar = ({
  variant = 'dark',
  isDocs = false,
}: NavbarProps): React.ReactElement => {
  const [isMobileMenuOpen, setIsMobileMenuOpen] = useState(false)
  const isLight = variant === 'light'

  return (
    <nav className={`${isDocs ? 'relative w-full' : 'absolute top-0 left-0 right-0 z-50'} px-8 py-6 md:px-12 4k:px-16 4k:py-10`}>
      <div className="flex items-center justify-between max-w-[1376px] 4k:max-w-none mx-auto 4k:mx-0">
        <Link href="/" className="flex items-center">
          <Image
            src={isLight ? '/simpleBLE-logo-dark.png' : '/simpleBLE-logo.png'}
            alt="SimpleBLE"
            width={152}
            height={50}
            className="h-12 4k:h-16 w-auto"
            priority
          />
        </Link>

        {!isDocs && (
          <div className="hidden md:flex items-center gap-3 4k:gap-4">
            <Link
              href="/pricing"
              className={`px-[18px] py-[14px] 4k:px-6 4k:py-4 border rounded-2xl font-sora font-semibold text-base 4k:text-lg tracking-[-0.32px] transition-colors ${
                isLight
                  ? 'border-primary/30 text-primary hover:bg-primary/10'
                  : 'border-white/30 text-white hover:bg-white/10'
              }`}
            >
              See Pricing
            </Link>
            <Link
              href="/try"
              className={`px-[18px] py-[14px] 4k:px-6 4k:py-4 rounded-2xl font-sora font-semibold text-base 4k:text-lg tracking-[-0.32px] transition-colors ${
                isLight
                  ? 'bg-primary text-white hover:bg-primary/90'
                  : 'bg-white text-primary hover:bg-white/90'
              }`}
            >
              Try for free
            </Link>
          </div>
        )}

        {!isDocs && (
          <button
            className={`md:hidden p-2 ${isLight ? 'text-primary' : 'text-white'}`}
            onClick={() => setIsMobileMenuOpen(!isMobileMenuOpen)}
            aria-label="Toggle menu"
          >
            <svg
              className="w-6 h-6"
              fill="none"
              stroke="currentColor"
              viewBox="0 0 24 24"
            >
              {isMobileMenuOpen ? (
                <path
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  strokeWidth={2}
                  d="M6 18L18 6M6 6l12 12"
                />
              ) : (
                <path
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  strokeWidth={2}
                  d="M4 6h16M4 12h16M4 18h16"
                />
              )}
            </svg>
          </button>
        )}
      </div>

      {!isDocs && (
        <div
          className={`md:hidden overflow-hidden transition-all duration-300 ease-in-out -mx-8 px-8 ${
            isMobileMenuOpen ? 'max-h-[200px] opacity-100' : 'max-h-0 opacity-0'
          }`}
        >
          <div
            className={`mt-4 flex flex-col gap-3 w-full p-4 rounded-2xl ${
              isLight ? 'bg-white shadow-lg' : 'bg-primary/95 backdrop-blur-sm'
            }`}
          >
            <Link
              href="/pricing"
              className={`px-[18px] py-[14px] border rounded-2xl font-sora font-semibold text-base tracking-[-0.32px] text-center transition-colors ${
                isLight
                  ? 'border-primary/30 text-primary hover:bg-primary/10'
                  : 'border-white/30 text-white hover:bg-white/10'
              }`}
            >
              See Pricing
            </Link>
            <Link
              href="/try"
              className={`px-[18px] py-[14px] rounded-2xl font-sora font-semibold text-base tracking-[-0.32px] text-center transition-colors ${
                isLight
                  ? 'bg-primary text-white hover:bg-primary/90'
                  : 'bg-white text-primary hover:bg-white/90'
              }`}
            >
              Try for free
            </Link>
          </div>
        </div>
      )}
    </nav>
  )
}
