use clap::Parser;

/// Display dimensions
#[derive(Debug, Clone, Copy)]
pub struct Dimensions {
    pub width: u32,
    pub height: u32,
}

impl Dimensions {
    pub fn new(width: u32, height: u32) -> Self {
        Self { width, height }
    }

    pub fn is_portrait(&self) -> bool {
        self.height > self.width
    }

    pub fn smaller_dimension(&self) -> u32 {
        self.width.min(self.height)
    }

    pub fn larger_dimension(&self) -> u32 {
        self.width.max(self.height)
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

/// UI Layout constants
pub mod layout {
    /// Number of buttons in smaller dimension
    pub const BUTTONS_SMALL_DIM: i32 = 2;
    
    /// Number of buttons in larger dimension
    pub const BUTTONS_LARGE_DIM: i32 = 4;
    
    /// Padding as percentage of smaller dimension
    pub const PADDING_PERCENT: f32 = 0.05;
    
    /// Button spacing as percentage of smaller dimension
    pub const SPACING_PERCENT: f32 = 0.03;
    
    /// Font sizes as percentage of button size
    pub const FONT_SIZE_BASE: f32 = 0.15;
    pub const FONT_SIZE_LARGE: f32 = 0.20;
    pub const FONT_SIZE_SMALL: f32 = 0.12;
    
    /// Swipe threshold as percentage of screen width
    pub const SWIPE_THRESHOLD_PERCENT: f32 = 0.15;
    
    /// Minimum swipe distance in pixels
    pub const SWIPE_MIN_DISTANCE: f32 = 50.0;
}