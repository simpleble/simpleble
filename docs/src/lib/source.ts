import { type InferPageType, loader } from 'fumadocs-core/source';
import { lucideIconsPlugin } from 'fumadocs-core/source/lucide-icons';
import { docs } from 'fumadocs-mdx:collections/server';

const docsSource = docs.toFumadocsSource();

// See https://fumadocs.dev/docs/headless/source-api for more info
export const source = loader({
  baseUrl: '',
  source: docsSource,
  plugins: [lucideIconsPlugin()],
});

// Each library and binding has a top-level folder, such as simplepyble/, whose
// meta.json title is its name.
const bindingNames = new Map<string, string>();
for (const file of docsSource.files) {
  const folder = file.type === 'meta' && /^(simple\w+)\/meta\.json$/.exec(file.path);
  if (folder && file.data.title) bindingNames.set(folder[1], file.data.title);
}

// Bindings share page titles such as "Usage" and "API Reference", so the title
// search engines and agents see names the binding: "SimplePyBLE Usage". Guides
// in deeper folders have descriptive titles and keep them.
export function getPageTitle(page: InferPageType<typeof source>) {
  const title = page.data.title;
  const binding = page.slugs.length === 2 ? bindingNames.get(page.slugs[0]) : undefined;
  if (!binding || title.includes(binding)) return title;

  return `${binding} ${title}`;
}

export async function getLLMText(page: InferPageType<typeof source>) {
  const processed = await page.data.getText('processed');

  return `# ${getPageTitle(page)}

${processed}`;
}
