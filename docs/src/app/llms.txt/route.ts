import { getPageTitle, source } from '@/lib/source';

export const revalidate = false;

export async function GET() {
  const lines: string[] = [];
  lines.push('# Documentation');
  lines.push('');
  for (const page of source.getPages()) {
    lines.push(`- [${getPageTitle(page)}](${page.url}): ${page.data.description}`);
  }
  return new Response(lines.join('\n'));
}
