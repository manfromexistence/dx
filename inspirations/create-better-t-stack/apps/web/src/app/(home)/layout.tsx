"use client";

import { HomeLayout } from "fumadocs-ui/layouts/home";
import { usePathname } from "next/navigation";
import { type ReactNode, useEffect } from "react";
import { baseOptions } from "@/app/layout.config";

export default function Layout({ children }: { children: ReactNode }) {
	const pathname = usePathname();

	useEffect(() => {
		const header = document.querySelector("#nd-nav");
		if (!header) return;

		const main = document.querySelector("main");
		if (!main) return;

		if (pathname === "/new") {
			header.classList.remove("*:mx-auto", "*:max-w-fd-container");
		} else {
			header.classList.add("*:mx-auto", "*:max-w-fd-container");
		}
	}, [pathname]);

	return (
		<HomeLayout
			{...baseOptions}
			style={
				{
					"--spacing-fd-container": "1280px",
				} as object
			}
		>
			<main className="h-full w-full">{children}</main>
		</HomeLayout>
	);
}
