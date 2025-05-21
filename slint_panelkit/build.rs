fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("cargo:rerun-if-changed=ui/");
    
    // Compile the main UI file
    slint_build::compile_with_config(
        "ui/main.slint",
        slint_build::CompilerConfiguration::new()
            .with_style("fluent".to_string())
    )?;
    
    // We don't need to compile component files directly,
    // as they are imported in the main UI file
    
    Ok(())
}