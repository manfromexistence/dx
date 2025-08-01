fn main() {
    println!("cargo:rerun-if-changed=src/generator/main.c");

    cc::Build::new()
        .file("src/generator/main.c")
        .compile("generator");
}