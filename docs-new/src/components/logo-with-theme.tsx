'use client'

import Image from 'next/image'
import { useEffect, useState } from 'react'

type LogoWithThemeProps = {
  width: number
  height: number
  className?: string
  priority?: boolean
}

export function LogoWithTheme({
  width,
  height,
  className,
  priority = false,
}: LogoWithThemeProps): React.ReactElement {
  const [isDark, setIsDark] = useState(false)
  const [mounted, setMounted] = useState(false)

  useEffect(() => {
    setMounted(true)
    
    // Check if dark mode is active by looking for .dark class on html element
    const checkDarkMode = () => {
      setIsDark(document.documentElement.classList.contains('dark'))
    }

    // Initial check
    checkDarkMode()

    // Watch for changes using MutationObserver
    const observer = new MutationObserver(checkDarkMode)
    observer.observe(document.documentElement, {
      attributes: true,
      attributeFilter: ['class'],
    })

    return () => observer.disconnect()
  }, [])

  // Use white logo (simpleBLE-logo.png) for dark mode
  // Use dark logo (simpleBLE-logo-dark.png) for light mode
  const logoSrc = mounted && isDark ? '/simpleBLE-logo.png' : '/simpleBLE-logo-dark.png'

  return (
    <Image
      src={logoSrc}
      alt="SimpleBLE"
      width={width}
      height={height}
      className={className}
      priority={priority}
    />
  )
}
