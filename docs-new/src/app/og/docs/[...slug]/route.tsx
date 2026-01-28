import { source } from '@/lib/source';
import { notFound } from 'next/navigation';
import { generate as MetadataImage, getImageResponseOptions } from './generate';
import { ImageResponse } from 'next/og';
import { getPageImage } from '@/lib/metadata';

export const revalidate = false;

export async function GET(_req: Request, { params }: { params: Promise<{ slug: string[] }> }) {
  const { slug } = await params;
  // Remove the trailing 'image.webp' from the slug to find the page
  const page = source.getPage(slug.slice(0, -1));
  if (!page) notFound();

  const logoUrl = new URL("/simpleBLE-logo-dark.png", _req.url).toString();

  return new ImageResponse(
    <MetadataImage 
      title={page.data.title} 
      description={page.data.description} 
      logoUrl={logoUrl}
    />,
    await getImageResponseOptions(),
  );
}

// Pre-generate images at build time
export function generateStaticParams() {
  return source.getPages().map((page) => ({
    slug: getPageImage(page).segments,
  }));
}
