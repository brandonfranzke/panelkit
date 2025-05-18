use log::{info, LevelFilter};
use simple_logger::SimpleLogger;
use std::env;

// Import UI from external file
slint::include_modules!();

// Define a simple exit function that can be called from outside
#[no_mangle]
pub extern "C" fn quit_app() {
    info!("Quit function called - exiting application");
    std::process::exit(0);
}

fn print_usage(program: &str) {
    eprintln!("Usage: {} [OPTIONS]", program);
    eprintln!("");
    eprintln!("OPTIONS:");
    eprintln!("  --width <WIDTH>          Set window width in pixels");
    eprintln!("  --height <HEIGHT>        Set window height in pixels");
    eprintln!("  --help                   Display this help message");
    eprintln!("");
    eprintln!("Examples:");
    eprintln!("  {} --width 640 --height 480", program);
    eprintln!("  {} --width 480 --height 640", program);
    eprintln!("");
    eprintln!("Note: Orientation (portrait/landscape) is automatically detected");
    eprintln!("      based on width vs height. Width < Height = Portrait mode.");
}

fn main() -> Result<(), slint::PlatformError> {
    // Minimal logging setup
    SimpleLogger::new()
        .with_level(LevelFilter::Info)
        .init()
        .unwrap();

    info!("Starting application");
    
    // Parse command line arguments
    let args: Vec<String> = env::args().collect();
    let mut width = None;
    let mut height = None;
    
    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "--width" => {
                if i + 1 < args.len() {
                    width = args[i + 1].parse::<u32>().ok();
                    i += 2;
                } else {
                    eprintln!("Error: --width requires a value");
                    std::process::exit(1);
                }
            }
            "--height" => {
                if i + 1 < args.len() {
                    height = args[i + 1].parse::<u32>().ok();
                    i += 2;
                } else {
                    eprintln!("Error: --height requires a value");
                    std::process::exit(1);
                }
            }
            "--help" => {
                print_usage(&args[0]);
                std::process::exit(0);
            }
            _ => {
                eprintln!("Error: Unknown option '{}'", args[i]);
                print_usage(&args[0]);
                std::process::exit(1);
            }
        }
    }
    
    // Validate dimensions if provided
    if (width.is_some() && height.is_none()) || (width.is_none() && height.is_some()) {
        eprintln!("Error: Both --width and --height must be provided together");
        print_usage(&args[0]);
        std::process::exit(1);
    }
    
    // Log configuration
    if let (Some(w), Some(h)) = (width, height) {
        info!("Using dimensions: {}x{}", w, h);
        if h > w {
            info!("Detected orientation: portrait (height > width)");
        } else {
            info!("Detected orientation: landscape (width >= height)");
        }
    }
    
    // For embedded mode, check root permissions
    #[cfg(all(feature = "embedded", target_family = "unix"))]
    {
        if unsafe { libc::geteuid() } != 0 {
            info!("WARNING: Not root. LinuxKMS noseat requires root");
        }
    }

    // Create window
    let window = MainWindow::new()?;
    
    // Set window size if provided via command line
    if let (Some(w), Some(h)) = (width, height) {
        // For development, set the window size
        #[cfg(not(feature = "embedded"))]
        {
            window.window().set_size(slint::PhysicalSize::new(w, h));
            info!("Window size set to: {}x{}", w, h);
        }
    }
    
    // Note: The Quit button changes the background to red
    // and we can't hook up the callback directly
    // Press ESC to exit
    
    // Run the application
    window.run()
}