# Thank you https://github.com/belgattitude/nextjs-monorepo-example!

# TODO (sessamekesh): Apply monolith build strategy from
#  https://cjolowicz.github.io/posts/incremental-docker-builds-for-monolithic-codebases/
#  to get cmake incremental builds working (instead of re-building everything!)

#
# Stage 1: Install all workspaces (dev)dependencies and generate node_modules folder(s)
#
FROM node:16-alpine3.15 AS ts-deps
RUN apk add --no-cache rsync

WORKDIR /workspace-install

COPY ts/yarn.lock ./

RUN --mount=type=bind,source=ts,target=/docker-context \
    rsync -amv -delete \
          --exclude='node_modules' \
          --exclude='*/node_modules' \
          --include='package.json' \
          --include='*/' --exclude='*' \
        /docker-context/ /workspace-install/

RUN --mount=type=cache,target=/root/.yarn3-cache,id=yarn3-cache \
    YARN_CACHE_FOLDER=/root/.yarn3-cache \
    yarn install --immutable --inline-builds

#
# Optional: develop web frontend
#
FROM node:16-alpine3.15 AS develop
ENV NODE_ENV=development

WORKDIR /app

# TS
COPY --from=ts-deps /workspace-install ./

EXPOSE 3000

WORKDIR /app/webmain
CMD ["yarn", "dev"]
