import { Banner } from "fumadocs-ui/components/banner";
import { DocsLayout } from "fumadocs-ui/layouts/docs";
import type { ReactNode } from "react";
import { baseOptions } from "@/app/layout.config";
import { source } from "@/lib/source";

export default function Layout({ children }: { children: ReactNode }) {
	return (
		<>
			<Banner variant="rainbow">
				⚠️ WORK IN PROGRESS DONT TAKE REFERENCE!!!
			</Banner>
			<DocsLayout tree={source.pageTree} {...baseOptions}>
				{children}
			</DocsLayout>
		</>
	);
}
