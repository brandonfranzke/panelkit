use anyhow::Result;
use crate::utils::config::AppConfig;

pub struct PlatformAdapter {
    is_embedded: bool,
}

impl PlatformAdapter {
    pub fn new(config: &AppConfig) -> Result<Self> {
        Ok(Self { is_embedded: config.embedded })
    }

    pub fn check_permissions(&self) -> Result<()> {
        if self.is_embedded {
            self.check_embedded_permissions()
        } else {
            Ok(())
        }
    }

    #[cfg(all(feature = "embedded", target_family = "unix"))]
    fn check_embedded_permissions(&self) -> Result<()> {
        let uid = unsafe { libc::geteuid() };
        if uid != 0 {
            log::warn!("Not running as root. LinuxKMS backend may require root permissions");
        }
        Ok(())
    }

    #[cfg(not(all(feature = "embedded", target_family = "unix")))]
    fn check_embedded_permissions(&self) -> Result<()> {
        Ok(())
    }
    
    pub fn configure_display(&self, config: &AppConfig) {
        // Always hide scrollbars
        std::env::set_var("SLINT_NO_SCROLLBAR", "1");
        
        log::info!(
            "Configuring display: {}x{} ({})", 
            config.window_width(), 
            config.window_height(),
            if config.is_portrait() { "portrait" } else { "landscape" }
        );
        
        // Set environment variables based on backend
        if self.is_embedded {
            #[cfg(feature = "embedded")]
            {
                std::env::set_var("SLINT_FULLSCREEN", "1");
                std::env::set_var("SLINT_BACKEND", "linuxkms");
                std::env::set_var("SLINT_RENDERER", "software");
                std::env::set_var("SLINT_FORCE_NOSEAT", "1");
                
                log::info!("Configured embedded display using LinuxKMS backend");
            }
        } else {
            // Desktop configuration
            log::info!("Configured desktop display");
        }
    }
}