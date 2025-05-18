use clap::Parser;

/// Display dimensions
#[derive(Debug, Clone, Copy)]
#[allow(dead_code)] // These fields are only used on desktop builds
pub struct Dimensions {
    pub width: u32,
    pub height: u32,
}

#[allow(dead_code)] // Only used on desktop builds
impl Dimensions {
    pub fn new(width: u32, height: u32) -> Self {
        Self { width, height }
    }
    
    #[cfg(not(feature = "embedded"))]
    pub fn is_portrait(&self) -> bool {
        self.height > self.width
    }
}

/// Command line arguments
#[derive(Parser, Debug)]
#[command(name = "slint_panelkit")]
#[command(about = "PanelKit UI Application", long_about = None)]
pub struct Args {
    /// Display width in pixels
    #[arg(long)]
    pub width: Option<u32>,

    /// Display height in pixels
    #[arg(long)]
    pub height: Option<u32>,

    /// Enable debug logging level
    #[arg(long)]
    pub debug: bool,
}

/// Application configuration
#[derive(Debug, Clone)]
pub struct AppConfig {
    pub dimensions: Option<Dimensions>,
    pub is_embedded: bool,
    pub debug_logging: bool,
}

impl AppConfig {
    pub fn from_args(args: Args) -> Self {
        let dimensions = match (args.width, args.height) {
            (Some(w), Some(h)) => Some(Dimensions::new(w, h)),
            (None, None) => None,
            _ => {
                eprintln!("Error: Both --width and --height must be provided together");
                std::process::exit(1);
            }
        };

        #[cfg(feature = "embedded")]
        let is_embedded = true;
        #[cfg(not(feature = "embedded"))]
        let is_embedded = false;

        Self {
            dimensions,
            is_embedded,
            debug_logging: args.debug,
        }
    }
}

