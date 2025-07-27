"use client";

import { motion } from "motion/react";
import { Suspense } from "react";
import StackBuilder from "../_components/stack-builder";

export default function FullScreenStackBuilder() {
	return (
		<Suspense>
			<motion.div
				initial={{ opacity: 0 }}
				animate={{ opacity: 1 }}
				transition={{ duration: 0.3 }}
				className="grid h-[calc(100vh-64px)] w-full flex-1 grid-cols-1 overflow-hidden"
			>
				<StackBuilder />
			</motion.div>
		</Suspense>
	);
}
