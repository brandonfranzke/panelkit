use anyhow::Result;

pub struct PlatformAdapter {
    is_embedded: bool,
}

impl PlatformAdapter {
    pub fn new(is_embedded: bool) -> Result<Self> {
        Ok(Self { is_embedded })
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
}

/// Platform-specific display initialization
pub fn configure_display() {
    #[cfg(feature = "embedded")]
    {
        std::env::set_var("SLINT_FULLSCREEN", "1");
        std::env::set_var("SLINT_BACKEND", "linuxkms");
        std::env::set_var("SLINT_RENDERER", "software");
        std::env::set_var("SLINT_FORCE_NOSEAT", "1");
    }
}