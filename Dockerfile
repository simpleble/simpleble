# Use Node.js 20 Alpine as base image for smaller size
FROM node:20-alpine AS base

# Install dependencies only when needed
FROM base AS deps
RUN apk add --no-cache libc6-compat
WORKDIR /app

# Copy package files from docs-new (context is now project root)
COPY docs-new/package.json docs-new/package-lock.json ./
# Install dependencies WITHOUT running postinstall scripts
RUN npm ci --ignore-scripts

# Rebuild the source code only when needed
FROM base AS builder

# Install Doxygen for documentation generation
RUN apk add --no-cache doxygen

WORKDIR /app

# Copy source files needed by Doxygen
COPY simpleble/include ./simpleble/include
COPY dependencies/external ./dependencies/external

# Copy docs-new content
COPY docs-new ./docs-new

# Copy node_modules from deps stage
COPY --from=deps /app/node_modules ./docs-new/node_modules

# Change to docs-new directory for build (so Doxyfile relative paths work)
WORKDIR /app/docs-new

ENV NEXT_TELEMETRY_DISABLED=1

# Build the application (includes Doxygen)
RUN npm run build

# Clean up source files to reduce image size
RUN rm -rf /app/simpleble /app/dependencies

# Production image, copy all the files and run next
FROM base AS runner
WORKDIR /app

ENV NODE_ENV=production
ENV NEXT_TELEMETRY_DISABLED=1

RUN addgroup --system --gid 1001 nodejs
RUN adduser --system --uid 1001 nextjs

# Copy the built application from docs-new
COPY --from=builder /app/docs-new/public ./public

# Set the correct permission for prerender cache
RUN mkdir .next
RUN chown nextjs:nodejs .next

# Automatically leverage output traces to reduce image size
COPY --from=builder --chown=nextjs:nodejs /app/docs-new/.next/standalone ./
COPY --from=builder --chown=nextjs:nodejs /app/docs-new/.next/static ./.next/static

USER nextjs

EXPOSE 3000

ENV PORT=3000
ENV HOSTNAME="0.0.0.0"

CMD ["node", "server.js"]