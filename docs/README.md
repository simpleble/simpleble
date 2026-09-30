# SimpleBLE Documentation

This is the documentation website for [SimpleBLE](https://github.com/simpleble/simpleble), a cross-platform Bluetooth Low Energy (BLE) library.

Built with [Fumadocs](https://fumadocs.dev) and [Next.js](https://nextjs.org).

## Getting Started

Run the development server:

```bash
npm run dev
```

Open [http://localhost:3000](http://localhost:3000) with your browser to see the result.

## API Reference

`content/docs/simpleble/api.mdx` is maintained by hand. It is not generated from the C++ sources, so update it whenever the public API changes.

It is written with the `<ApiSection>`, `<ApiClass>` and `<ApiMethod>` components from `src/components/api/`. The `type` of each `ApiMethod` parameter is rendered as highlighted C++ code, so it must be plain C++ with no Markdown links.

## Project Structure

| Route                | Description                                        |
| -------------------- | -------------------------------------------------- |
| `app/(home)`         | The landing page.                                  |
| `app/(docs)`         | The documentation layout and pages.                |
| `content/docs`       | MDX/Markdown files for the documentation.          |
| `src/components/api` | Custom React components used in the API reference. |

## Learn More

- [Fumadocs Documentation](https://fumadocs.dev)
- [Next.js Documentation](https://nextjs.org/docs)
- [SimpleBLE Repository](https://github.com/simpleble/simpleble)
