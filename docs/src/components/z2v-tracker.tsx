'use client';

import Script from 'next/script';
import { usePathname } from 'next/navigation';
import { useCallback, useEffect, useRef, useState } from 'react';

const Z2V_CLIENT_SRC = 'https://t.z2v.org/client/track.js';

type Z2VTrackApi = Readonly<{
  init: (options: Readonly<{ consent: boolean; pageView: boolean }>) => void;
  track: (type: string) => Promise<Response>;
}>;

declare global {
  interface Window {
    Z2VTrack?: Z2VTrackApi;
  }
}

export function Z2VTracker() {
  const pathname = usePathname();
  const initialized = useRef(false);
  const [ready, setReady] = useState(false);

  const initialize = useCallback(() => {
    if (initialized.current || !window.Z2VTrack) return;

    window.Z2VTrack.init({ consent: true, pageView: false });
    initialized.current = true;
    setReady(true);
  }, []);

  useEffect(() => {
    if (!ready) return;

    void window.Z2VTrack?.track('page_view').catch(() => undefined);
  }, [pathname, ready]);

  return (
    <Script
      src={Z2V_CLIENT_SRC}
      strategy="afterInteractive"
      onReady={initialize}
    />
  );
}
