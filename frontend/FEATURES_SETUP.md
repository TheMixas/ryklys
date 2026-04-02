# Feature Development Quick Start

## ✅ What I've Set Up For You

1. **ESLint Plugin** - Installed `eslint-plugin-import`
2. **Import Restrictions** - Configured strict rules to prevent cross-feature imports
3. **Feature Structure** - Created 7 new features with proper folder structure:
   - `viewer-chat` - Chat for viewers
   - `stream` - Video playback
   - `channel` - User profiles
   - `discovery` - Browse streams
   - `follow` - Follow system
   - `stream-settings` - Stream config
   - `analytics` - Stats & metrics

4. **Index Files** - Set up public API pattern for all features

## 🚀 Getting Started

### 1. Start with Auth (Already exists)
Make sure your auth feature exports what you need:
```typescript
// src/features/auth/index.ts
export { LoginForm } from './components/LoginForm';
export { useAuth } from './hooks/useAuth';
```

### 2. Build the Broadcast Feature (Already exists)
This is your unique selling point! Focus on:
- WebRTC setup
- MediaStream API
- Screen/camera capture
- Stream configuration

### 3. Create the Stream Player
```typescript
// src/features/stream/components/VideoPlayer.tsx
export function VideoPlayer({ streamUrl }: { streamUrl: string }) {
  // HLS/WebRTC video player implementation
}

// src/features/stream/index.ts
export { VideoPlayer } from './components/VideoPlayer';
```

### 4. Compose in App Layer
```typescript
// src/app/routes/WatchStream.tsx
import { VideoPlayer } from '@/features/stream';
import { ViewerChat } from '@/features/channel-chat';
import { ChannelInfo } from '@/features/channel';
import { FollowButton } from '@/features/follow';

export function WatchStreamPage() {
  return (
    <div className="watch-page">
      <VideoPlayer streamUrl="..." />
      <ChannelInfo channelId="..." />
      <FollowButton userId="..." />
      <ViewerChat streamId="..." />
    </div>
  );
}
```

## 📋 Development Order (Recommended)

### Phase 1 - MVP
1. ✅ **auth** - Login/register
2. ✅ **broadcast** - Stream from browser
3. **stream** - Watch streams
4. **viewer-chat** - Basic chat

### Phase 2 - Core Features
5. **channel** - User profiles
6. **discovery** - Browse/search
7. **follow** - Follow users

### Phase 3 - Advanced
8. **stream-settings** - Advanced controls
9. **streamer-chat** - Moderation tools
10. **analytics** - Stats dashboard

## 🔒 Import Rules

### ✅ Allowed
```typescript
// Features can import from shared modules
import { Button } from '@/components/Button';
import { useLocalStorage } from '@/hooks/useLocalStorage';
import type { User } from '@/types/user';

// App can import from features
import { StreamPlayer } from '@/features/stream';
```

### ❌ Not Allowed (ESLint will error)
```typescript
// Features CANNOT import from other features
import { ChatMessage } from '@/features/channel-chat'; // ❌ ERROR

// Features CANNOT import from app
import { router } from '@/app/router'; // ❌ ERROR

// Shared modules CANNOT import from features/app
import { useAuth } from '@/features/auth'; // ❌ ERROR in shared hook
```

## 💡 Tips

1. **Keep features small** - If a feature gets too large, split it
2. **Use shared stores** - For cross-feature state (Zustand, Jotai, etc.)
3. **Composition over props drilling** - Let the app layer handle coordination
4. **Test features independently** - Each should be testable in isolation

## 🧪 Test Your Setup

Try creating a component that violates the rules:

```typescript
// src/features/stream/components/TestViolation.tsx
import { ViewerChat } from '@/features/channel-chat'; // ❌ Should error!

// Run: npm run lint
```

ESLint will catch it! 🎉

## 📚 More Info

See `src/features/ARCHITECTURE.md` for detailed architecture documentation.

