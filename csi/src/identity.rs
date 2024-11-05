use crate::csi::v1::identity_server::Identity;
use crate::csi::v1::*;
use std::collections::HashMap;
use tonic::{Request, Response, Status};

pub struct IdentityService {
    name: String,
}

impl IdentityService {
    pub fn new(name: &str) -> Self {
        return Self {
            name: name.to_string(),
        };
    }
}

#[tonic::async_trait]
impl Identity for IdentityService {
    async fn probe(&self, _: Request<ProbeRequest>) -> Result<Response<ProbeResponse>, Status> {
        Ok(Response::new(ProbeResponse { ready: Some(true) }))
    }
    async fn get_plugin_info(
        &self,
        _: Request<GetPluginInfoRequest>,
    ) -> Result<Response<GetPluginInfoResponse>, Status> {
        Ok(Response::new(GetPluginInfoResponse {
            name: self.name.clone(),
            vendor_version: env!("CARGO_PKG_VERSION").to_string(),
            manifest: HashMap::new(),
        }))
    }
    async fn get_plugin_capabilities(
        &self,
        _: Request<GetPluginCapabilitiesRequest>,
    ) -> Result<Response<GetPluginCapabilitiesResponse>, Status> {
        Ok(Response::new(GetPluginCapabilitiesResponse {
            // TODO: advertise additional capabilities as they are implemented
            capabilities: vec![],
        }))
    }
}
