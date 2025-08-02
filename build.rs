fn main() {
    println!("cargo:rerun-if-changed=c/main.c");

    cc::Build::new()
        .file("c/main.c")
        .compile("generator");
    println!("cargo:rustc-link-lib=pthread");
}
