import { existsSync, readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';
import { createMDX } from 'fumadocs-mdx/next';

const withMDX = createMDX();

const CONTENT_DIR = join(import.meta.dirname, 'content/docs');

// Pages that moved, and code paths in the docs that crawlers requested as URLs.
const MOVED_PAGES = [
  ['/ble_basics', '/fundamentals/ble_basics'],
  ['/cmake_primer', '/fundamentals/cmake_primer'],
  ['/concurrency', '/fundamentals/concurrency'],
  ['/error-handling', '/fundamentals/error-handling'],
  ['/permissions', '/fundamentals/permissions'],
  ['/examples/simpleble', '/simpleble/examples'],
  ['/examples/simplebluez', '/simplebluez/examples'],
  ['/simpleble/build/simpleble_quickstart', '/simpleble/tutorial'],
  ['/simpleble/build_simpleble_test/bin/simpleble_test', '/simpleble/usage'],
];

// A folder without an index.mdx has no page of its own, so its URL goes to the
// first page its meta.json lists, as /simplepyble does to /simplepyble/usage.
function folderRedirects(dir = CONTENT_DIR, url = '') {
  return readdirSync(dir, { withFileTypes: true })
    .filter((entry) => entry.isDirectory())
    .flatMap((entry) => {
      const folder = join(dir, entry.name);
      const source = `${url}/${entry.name}`;
      const nested = folderRedirects(folder, source);
      if (existsSync(join(folder, 'index.mdx'))) return nested;

      const meta = JSON.parse(readFileSync(join(folder, 'meta.json'), 'utf8'));
      return [[source, `${source}/${meta.pages[0]}`], ...nested];
    });
}

// HSTS leaves out includeSubDomains and preload: other simpleble.org hosts are
// not managed here, and preload is hard to undo.
const SECURITY_HEADERS = [
  { key: 'Strict-Transport-Security', value: 'max-age=63072000' },
  { key: 'X-Content-Type-Options', value: 'nosniff' },
  { key: 'Referrer-Policy', value: 'strict-origin-when-cross-origin' },
  { key: 'X-Frame-Options', value: 'SAMEORIGIN' },
  { key: 'Content-Security-Policy', value: "frame-ancestors 'self'" },
  {
    key: 'Permissions-Policy',
    value: 'camera=(), microphone=(), geolocation=()',
  },
];

/** @type {import('next').NextConfig} */
const config = {
  reactStrictMode: true,
  output: 'standalone',
  poweredByHeader: false,
  images: {
    // Serve existing images without letting crawlers trigger server-side resizing.
    unoptimized: true,
  },
  async headers() {
    // The raw Markdown copies stay readable for agents but out of search results.
    const noindex = [{ key: 'X-Robots-Tag', value: 'noindex' }];

    return [
      { source: '/:path*', headers: SECURITY_HEADERS },
      { source: '/:path*.mdx', headers: noindex },
      { source: '/llms.mdx/:path*', headers: noindex },
    ];
  },
  async redirects() {
    const moved = [...MOVED_PAGES, ...folderRedirects()];

    return [
      ...moved.flatMap(([from, to]) => [
        { source: from, destination: to, permanent: true },
        // Skip the second hop through the /docs/:path* rule below.
        { source: `/docs${from}`, destination: to, permanent: true },
      ]),
      {
        source: '/docs',
        destination: '/overview',
        permanent: true,
      },
      {
        source: '/docs/:path*',
        destination: '/:path*',
        permanent: true,
      },
    ];
  },
  async rewrites() {
    return [
      {
        source: '/:path*.mdx',
        destination: '/llms.mdx/:path*',
      },
    ];
  },
};

export default withMDX(config);
