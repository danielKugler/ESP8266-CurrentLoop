import { defineConfig } from 'vite'
import { ViteMinifyPlugin } from 'vite-plugin-minify'
import { viteSingleFile } from "vite-plugin-singlefile"

export default defineConfig({
	plugins: [
		// input https://www.npmjs.com/package/html-minifier-terser options
		ViteMinifyPlugin({}), viteSingleFile()
	],
	envDir: "env",
	publicDir: "public",
	server: {
		host: "0.0.0.0",
		port: 3200,
		watch: {
			followSymlinks: false,
		},
		open: true,
		cors: true,
	}
})