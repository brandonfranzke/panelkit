use rand::Rng;
use slint::Color;

/// Application state management
pub struct AppState {
    background_color: Color,
    current_page: i32,
}

impl AppState {
    pub fn new() -> Self {
        Self {
            background_color: Color::from_rgb_u8(0x22, 0x22, 0x22),
            current_page: 0,
        }
    }

    pub fn set_background_color(&mut self, color: Color) {
        self.background_color = color;
    }

    pub fn get_background_color(&self) -> Color {
        self.background_color
    }

    pub fn set_current_page(&mut self, page: i32) {
        self.current_page = page;
    }

    pub fn get_current_page(&self) -> i32 {
        self.current_page
    }

    pub fn generate_random_color(&mut self) {
        let mut rng = rand::thread_rng();
        self.background_color = Color::from_rgb_u8(
            rng.gen(),
            rng.gen(),
            rng.gen(),
        );
    }
}

impl Default for AppState {
    fn default() -> Self {
        Self::new()
    }
}