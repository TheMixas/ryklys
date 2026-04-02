import js from '@eslint/js'
import globals from 'globals'
import reactHooks from 'eslint-plugin-react-hooks'
import reactRefresh from 'eslint-plugin-react-refresh'
import tseslint from 'typescript-eslint'
import importPlugin from 'eslint-plugin-import'
import { defineConfig, globalIgnores } from 'eslint/config'

export default defineConfig([
  globalIgnores(['dist']),
  {
    files: ['**/*.{ts,tsx}'],
    extends: [
      js.configs.recommended,
      tseslint.configs.recommended,
      reactHooks.configs.flat.recommended,
      reactRefresh.configs.vite,
    ],
    languageOptions: {
      ecmaVersion: 2020,
      globals: globals.browser,
    },
    plugins: {
      import: importPlugin,
    },
    rules: {
      'import/no-restricted-paths': [
        'error',
        {
          zones: [
            // Disables cross-feature imports:
            // Each feature should be isolated and not import from other features
            {
              target: './src/features/auth',
              from: './src/features',
              except: ['./auth'],
            },
            {
              target: './src/features/broadcast',
              from: './src/features',
              except: ['./broadcast'],
            },
            {
              target: './src/features/streamer-chat',
              from: './src/features',
              except: ['./streamer-chat'],
            },
            {
              target: './src/features/channel-chat',
              from: './src/features',
              except: ['./channel-chat'],
            },
            {
              target: './src/features/stream',
              from: './src/features',
              except: ['./stream'],
            },
            {
              target: './src/features/channel',
              from: './src/features',
              except: ['./channel'],
            },
            {
              target: './src/features/discovery',
              from: './src/features',
              except: ['./discovery'],
            },
            {
              target: './src/features/follow',
              from: './src/features',
              except: ['./follow'],
            },
            {
              target: './src/features/stream-settings',
              from: './src/features',
              except: ['./stream-settings'],
            },
            {
              target: './src/features/analytics',
              from: './src/features',
              except: ['./analytics'],
            },

            // Enforce unidirectional codebase:
            // src/app can import from src/features but not the other way around
            {
              target: './src/features',
              from: './src/app',
            },

            // Shared modules can't import from features or app
            // This keeps them truly reusable and prevents circular dependencies
            {
              target: [
                './src/components',
                './src/hooks',
                './src/stores',
                './src/types',
                './src/utility',
              ],
              from: ['./src/features', './src/app'],
            },
          ],
        },
      ],
    },
  },
])
