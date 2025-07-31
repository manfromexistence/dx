{ pkgs, ... }: {
  channel = "stable-24.05";
  packages = [
    pkgs.rustup
    pkgs.gcc
    pkgs.bun
    pkgs.tree
    pkgs.gnumake
    pkgs.libaio
  ];
  env = { };
  idx = {
    extensions = [
      "pkief.material-icon-theme"
      "ziglang.vscode-zig"
      "tamasfe.even-better-toml"
      "rust-lang.rust-analyzer"
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