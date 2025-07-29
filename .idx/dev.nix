{ pkgs, ... }: {
  channel = "stable-24.05";
  packages = [
    pkgs.rustup
    pkgs.gcc
    pkgs.bun
    pkgs.tree
    pkgs.gnumake
  ];
  env = { };
  idx = {
    extensions = [
      "pkief.material-icon-theme"
      "ziglang.vscode-zig"
      "tamasfe.even-better-toml"
      "rust-lang.rust-analyzer"
      "eamodio.gitlens"
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