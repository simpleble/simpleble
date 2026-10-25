import type { MetadataRoute } from 'next';

import { siteUrl } from '@/lib/site';
import { source } from '@/lib/source';

export default function sitemap(): MetadataRoute.Sitemap {
  return [
    { url: siteUrl },
    ...source.getPages().map((page) => ({ url: `${siteUrl}${page.url}` })),
  ];
}
