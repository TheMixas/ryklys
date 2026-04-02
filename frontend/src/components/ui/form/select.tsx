// import * as React from 'react';
// import type {UseFormRegisterReturn} from 'react-hook-form';
//
//
// import { FormItem, FormLabel, FormControl, FormMessage } from './form';
//
// type FieldLikeProps = {
//   label?: React.ReactNode;
//   error?: unknown;
// };
//
// type Option = {
//   label: React.ReactNode;
//   value: string | number | string[];
// };
//
// type SelectFieldProps = FieldLikeProps & {
//   options: Option[];
//   className?: string;
//   defaultValue?: string;
//   registration: Partial<UseFormRegisterReturn>;
// };
//
// const getErrorMessage = (err: unknown) => {
//   if (!err) return undefined;
//   if (typeof err === 'string') return err;
//   if (typeof err === 'object' && err !== null && 'message' in err) {
//     const e = err as { message?: unknown };
//     return e.message ? String(e.message) : undefined;
//   }
//   return String(err);
// };
//
// export const Select = (props: SelectFieldProps) => {
//   const { label, options, error, className, defaultValue, registration } =
//     props;
//
//   const message = getErrorMessage(error);
//
//   return (
//     <FormItem>
//       {label ? <FormLabel>{label}</FormLabel> : null}
//       <FormControl>
//         <select
//           className={cn(
//             'mt-1 block w-full rounded-md border-gray-600 py-2 pl-3 pr-10 text-base focus:border-blue-500 focus:outline-none focus:ring-blue-500 sm:text-sm',
//             className,
//           )}
//           defaultValue={defaultValue}
//           {...registration}
//         >
//           {options.map(({ label, value }) => (
//             <option key={label?.toString()} value={value}>
//               {label}
//             </option>
//           ))}
//         </select>
//       </FormControl>
//       {message ? <FormMessage>{message}</FormMessage> : null}
//     </FormItem>
//   );
// };
