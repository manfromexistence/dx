{ pkgs, ... }: {
  channel = "stable-24.05";
  packages = [
    pkgs.rustup
    pkgs.gcc
    pkgs.bun
  ];
  env = { };
  idx = {
    extensions = [
      "pkief.material-icon-theme"
      "ziglang.vscode-zig"
      "rust-lang.rust"
      "tamasfe.even-better-toml"
    ];
    workspace = {
      onCreate = {
        install = "rustup default stable && rustup update && cargo run";
        default.openFiles = [
          "README.md"
        ];
      };
    };
  };
}