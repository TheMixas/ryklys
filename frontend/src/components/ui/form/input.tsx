import * as React from 'react';
import { type UseFormRegisterReturn } from 'react-hook-form';

import { FormItem, FormLabel, FormControl, FormMessage } from './form';

export type FieldLikeProps = {
  label?: React.ReactNode;
  error?: unknown;
};

export type InputProps = React.InputHTMLAttributes<HTMLInputElement> &
  FieldLikeProps & {
    className?: string;
    registration?: Partial<UseFormRegisterReturn>;
  };

const getErrorMessage = (err: unknown) => {
  if (!err) return undefined;
  if (typeof err === 'string') return err;
  if (typeof err === 'object') {
    const e = err as { message?: unknown } | null;
    if (e && 'message' in e) return e.message ? String(e.message) : undefined;
  }
  return String(err);
};

const Input = React.forwardRef<HTMLInputElement, InputProps>(
  ({ className, type, label, error, registration, ...props }, ref) => {
    const message = getErrorMessage(error);

    // Local class concatenation to avoid needing `cn`/`clsx` helper
    const baseClasses =
      'flex h-9 w-full rounded-md border border-input bg-transparent px-3 py-1 text-sm shadow-sm transition-colors file:border-0 file:bg-transparent file:text-sm file:font-medium placeholder:text-muted-foreground focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring disabled:cursor-not-allowed disabled:opacity-50';
    const inputClasses = [baseClasses, className].filter(Boolean).join(' ');

    return (
      <FormItem>
        {label ? <FormLabel>{label}</FormLabel> : null}
        <FormControl>
          <input
            type={type}
            className={inputClasses}
            ref={ref}
            {...(registration || {})}
            {...props}
          />
        </FormControl>
        {message ? <FormMessage>{message}</FormMessage> : null}
      </FormItem>
    );
  },
);
Input.displayName = 'Input';

export { Input };
