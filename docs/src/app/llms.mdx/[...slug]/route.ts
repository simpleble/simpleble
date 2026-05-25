import { getLLMText, source } from '@/lib/source';
import { notFound } from 'next/navigation';

type MdxRouteProps = Readonly<{
  params: Promise<{
    slug: string[];
  }>;
}>;

export const revalidate = false;

export async function GET(_req: Request, { params }: MdxRouteProps) {
  const { slug } = await params;
  const page = source.getPage(slug);
  if (!page) notFound();

  return new Response(await getLLMText(page), {
    headers: {
      'Content-Type': 'text/markdown',
    },
  });
}

export function generateStaticParams() {
  return source.generateParams().filter(({ slug }) => slug.length > 0);
}
