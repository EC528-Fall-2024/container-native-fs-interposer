use crate::csi::v1::identity_server::Identity;
use crate::csi::v1::*;
use tonic::{Request, Response, Status};

pub struct IdentityServer {}

#[tonic::async_trait]
impl Identity for IdentityServer {
    async fn probe(&self, _: Request<ProbeRequest>) -> Result<Response<ProbeResponse>, Status> {
        unimplemented!()
    }
    async fn get_plugin_info(
        &self,
        _: Request<GetPluginInfoRequest>,
    ) -> Result<Response<GetPluginInfoResponse>, Status> {
        unimplemented!()
    }
    async fn get_plugin_capabilities(
        &self,
        _: Request<GetPluginCapabilitiesRequest>,
    ) -> Result<Response<GetPluginCapabilitiesResponse>, Status> {
        unimplemented!()
    }
}
