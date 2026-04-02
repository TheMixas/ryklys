# Feature Architecture

This project follows a feature-based architecture with strict import rules enforced by ESLint.

## Feature Structure

Each feature follows this structure:

```
features/
└── feature-name/
    ├── index.ts           # Public API (only export what's needed outside)
    ├── components/        # React components
    │   └── index.ts
    ├── hooks/             # Custom React hooks
    │   └── index.ts
    └── services/          # Business logic, API calls, utilities
```

## Features

### Core Features (MVP)
1. **auth** - User authentication & authorization
2. **broadcast** - Browser-based streaming (WebRTC, MediaStream API)
3. **stream** - Video player and stream playback for viewers
4. **streamer-chat** - Chat functionality for streamers (with moderation tools)
5. **viewer-chat** - Chat functionality for viewers

### Secondary Features
6. **channel** - User profiles, channel pages, stream history
7. **discovery** - Browse live streams, search, categories
8. **follow** - Follow/unfollow users, notifications
9. **stream-settings** - Stream configuration (bitrate, resolution, etc.)
10. **analytics** - Viewer statistics and metrics (optional)

## Architecture Rules

### 1. Feature Isolation
- Features **cannot** import from other features
- Each feature is self-contained
- ESLint will enforce this rule

❌ **Wrong:**
```typescript
// Inside src/features/stream/components/Player.tsx
import { ChatMessage } from '../../channel-chat/components/ChatMessage';
```

✅ **Correct:**
```typescript
// Inside src/features/stream/components/Player.tsx
import { Button } from '@/components/Button'; // shared component
```

### 2. Unidirectional Flow
- `src/app` can import from `src/features`
- `src/features` **cannot** import from `src/app`

✅ **Correct:**
```typescript
// Inside src/app/routes/StreamPage.tsx
import { StreamPlayer } from '@/features/stream';
import { ViewerChat } from '@/features/channel-chat';
```

### 3. Shared Modules
Shared code lives in:
- `src/components` - Reusable UI components
- `src/hooks` - Reusable React hooks
- `src/stores` - Global state management
- `src/types` - Shared TypeScript types
- `src/utility` - Helper functions

These **cannot** import from `src/features` or `src/app` to stay truly reusable.

## Development Guidelines

### Adding a New Feature
1. Create the folder structure: `features/new-feature/{components,hooks,services}`
2. Create `index.ts` files for each subfolder
3. Create the main `features/new-feature/index.ts` to export the public API
4. Add ESLint restriction in `eslint.config.js`

### Communication Between Features
If features need to share data, use:
- **Shared stores** in `src/stores`
- **Context providers** in `src/app/provider.tsx`
- **URL params/routing** for navigation
- **Custom events** for loose coupling

### Example: Stream Page Composition
```typescript
// src/app/routes/StreamPage.tsx
import { StreamPlayer } from '@/features/stream';
import { ViewerChat } from '@/features/channel-chat';
import { FollowButton } from '@/features/follow';
import { ChannelInfo } from '@/features/channel';

export function StreamPage() {
  return (
    <div>
      <StreamPlayer />
      <ChannelInfo />
      <FollowButton />
      <ViewerChat />
    </div>
  );
}
```

## Benefits
- ✅ Clear separation of concerns
- ✅ Prevents circular dependencies
- ✅ Easier to test and maintain
- ✅ Can extract features into separate packages later
- ✅ Multiple developers can work on different features without conflicts

