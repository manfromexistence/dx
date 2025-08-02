# Inspirations

charasay  create-better-t-stack  file_creator_instrumented                    footer  header   main    main.s    README.md  term_size-rs  tree_2.2.1.orig.tar.gz
clack     figlet-fonts           file_creator_instrumented-file_creator.gcda  git     lolcrab  main.c  platform  rs-figlet  tree-2.2.1

```
git clone https://github.com/torvalds/linux && cd linux && rm -rf .git && cd ..

git clone https://github.com/torvalds/linux && cd linux && rm -rf .git && cd ..
git clone https://github.com/git/git.git && cd git && rm -rf .git && cd ..
git clone https://github.com/biomejs/biome && cd biome && rm -rf .git && cd ..
git clone https://github.com/latipun7/charasay && cd charasay && rm -rf .git && cd ..
git clone https://github.com/yuanbohan/rs-figlet && cd rs-figlet && rm -rf .git && cd ..
git clone https://github.com/xero/figlet-fonts && cd rs-figlet && rm -rf .git && cd ..
git clone https://github.com/mazznoer/lolcrab && cd lolcrab && rm -rf .git && cd ..
git clone https://github.com/cross-rs/cross && cd cross && rm -rf .git && cd ..
git clone https://github.com/casey/just && cd just && rm -rf .git && cd ..
git clone https://github.com/console-rs/indicatif && cd indicatif && rm -rf .git && cd ..
git clone https://github.com/LinusU/rust-log-update && cd rust-log-update && rm -rf .git && cd ..
git clone https://github.com/VincentFoulon80/console_engine && cd console_engine && rm -rf .git && cd ..
git clone https://github.com/nukesor/comfy-table && cd comfy-table && rm -rf .git && cd ..
git clone https://github.com/manfromexistence/ui && cd ui && rm -rf .git && cd ..
git clone https://github.com/neovim/neovim && cd neovim && rm -rf .git && cd ..
git clone https://github.com/ghostty-org/ghostty && cd ghostty && rm -rf .git && cd ..
git clone https://github.com/redox-os/ion.git && cd ion && rm -rf .git && cd ..
git clone https://github.com/ohmyzsh/ohmyzsh && cd ohmyzsh && rm -rf .git && cd ..
git clone https://github.com/shadcn-ui/ui && cd claude-code && rm -rf .git && cd ..
git clone https://github.com/anthropics/claude-code && cd claude-code && rm -rf .git && cd ..
git clone https://github.com/ratatui/ratatui && cd ratatui && rm -rf .git && cd ..
git clone https://github.com/google-gemini/gemini-cli && cd gemini-cli && rm -rf .git && cd ..
git clone https://github.com/mikaelmello/inquire && cd inquire && rm -rf .git && cd ..
git clone https://github.com/bombshell-dev/clack && cd clack && rm -rf .git && cd ..
git clone https://github.com/oven-sh/bun && cd bun && rm -rf .git && cd ..
git clone https://github.com/haydenbleasel/ultracite.git && cd ultracite && rm -rf .git && cd ..
git clone https://github.com/tailwindlabs/tailwindcss && cd tailwindcss && rm -rf .git && cd ..
git clone https://github.com/AmanVarshney01/create-better-t-stack && cd create-better-t-stack && rm -rf .git && cd ..
git clone https://github.com/clap-rs/term_size-rs.git && cd term_size-rs && rm -rf .git && cd ..
```


```
[target.'cfg(not(target_os = "windows"))'.dependencies]
libc = "0.2.174"
[target.'cfg(target_os = "windows")'.dependencies]
winapi = { version = "0.3.9", features = ["wincon", "processenv", "winbase"] }

clap_complete = "4.5.42"
clap = { version = "4.5.27", features = ["derive", "wrap_help"], optional = true }
rust-embed = { version = "8.5.0", features = ["debug-embed"] }
textwrap = { version = "0.16.1", features = ["terminal_size"] }
unicode-width = "0.2.0"
regex = "1.11.1"
rand = "0.8.5"
strip-ansi-escapes = "0.2.1"

bstr = "1.9"
colorgrad = { version = "0.7" }
dirs = { version = "6.0", optional = true }
fastrand = "2.1"
mimalloc = { version = "0.1", optional = true, default-features = false }
noise = { version = "0.9", default-features = false }
shlex = { version = "1.3", optional = true }
unicode-segmentation = "1.10"
```