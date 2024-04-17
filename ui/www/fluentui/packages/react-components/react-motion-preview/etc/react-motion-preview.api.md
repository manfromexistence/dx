## API Report File for "@fluentui/react-motion-preview"

> Do not edit this file. It is a report generated by [API Extractor](https://api-extractor.com/).

```ts

import * as React_2 from 'react';

// @public
export function getDefaultMotionState<Element extends HTMLElement>(): MotionState<Element>;

// @public (undocumented)
export type MotionClassNames = {
    [key in MotionStylesKeys]?: string;
};

// @public (undocumented)
export type MotionOptions = {
    animateOnFirstMount?: boolean;
    duration?: number;
};

// @public (undocumented)
export type MotionShorthand<Element extends HTMLElement = HTMLElement> = MotionShorthandValue | MotionState<Element>;

// @public (undocumented)
export type MotionShorthandValue = boolean;

// @public (undocumented)
export type MotionState<Element extends HTMLElement = HTMLElement> = {
    ref: React_2.Ref<Element>;
    type: MotionType;
    canRender: boolean;
    active: boolean;
};

// @public (undocumented)
export type MotionStylesKeys = 'default' | 'enter' | 'exit' | MotionType;

// @public (undocumented)
export type MotionType = 'entering' | 'entered' | 'idle' | 'exiting' | 'exited' | 'unmounted';

// @public
export function useMotion<Element extends HTMLElement>(shorthand: MotionShorthand<Element>, options?: MotionOptions): MotionState<Element>;

// @public (undocumented)
export const useMotionClassNames: (motion: MotionState, motionStyles: MotionClassNames) => string;

// (No @packageDocumentation comment for this package)

```