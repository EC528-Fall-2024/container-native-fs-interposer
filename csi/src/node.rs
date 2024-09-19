use crate::csi::v1::node_server::Node;
use crate::csi::v1::*;
use tonic::{Request, Response, Status};
use uuid::Uuid;

pub struct NodePlugin {
    node_id: uuid::Uuid,
}

impl NodePlugin {
    pub fn new() -> Self {
        Self {
            node_id: Uuid::new_v4(),
        }
    }
}

#[tonic::async_trait]
impl Node for NodePlugin {
    async fn node_stage_volume(
        &self,
        _: Request<NodeStageVolumeRequest>,
    ) -> Result<Response<NodeStageVolumeResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_unstage_volume(
        &self,
        _: Request<NodeUnstageVolumeRequest>,
    ) -> Result<Response<NodeUnstageVolumeResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_publish_volume(
        &self,
        request: Request<NodePublishVolumeRequest>,
    ) -> Result<Response<NodePublishVolumeResponse>, Status> {
        let request = request.into_inner();
        if request.volume_id.is_empty() {
            return Err(Status::invalid_argument("volume_id is required"));
        }
        if request.target_path.is_empty() {
            return Err(Status::invalid_argument("target_path is required"));
        }
        if request.volume_capability.is_none() {
            return Err(Status::invalid_argument("volume_capability is required"));
        }
        // TODO: check volume_capability
        std::fs::create_dir(request.target_path)?;
        Ok(Response::new(NodePublishVolumeResponse {}))
    }
    async fn node_unpublish_volume(
        &self,
        request: Request<NodeUnpublishVolumeRequest>,
    ) -> Result<Response<NodeUnpublishVolumeResponse>, Status> {
        let request = request.into_inner();
        if request.volume_id.is_empty() {
            return Err(Status::invalid_argument("volume_id is required"));
        }
        if request.target_path.is_empty() {
            return Err(Status::invalid_argument("target_path is required"));
        }
        std::fs::remove_dir_all(request.target_path)?;
        Ok(Response::new(NodeUnpublishVolumeResponse {}))
    }
    async fn node_get_volume_stats(
        &self,
        _: Request<NodeGetVolumeStatsRequest>,
    ) -> Result<Response<NodeGetVolumeStatsResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_expand_volume(
        &self,
        _: Request<NodeExpandVolumeRequest>,
    ) -> Result<Response<NodeExpandVolumeResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_get_capabilities(
        &self,
        _: Request<NodeGetCapabilitiesRequest>,
    ) -> Result<Response<NodeGetCapabilitiesResponse>, Status> {
        Ok(Response::new(NodeGetCapabilitiesResponse {
            capabilities: vec![],
        }))
    }
    async fn node_get_info(
        &self,
        _: Request<NodeGetInfoRequest>,
    ) -> Result<Response<NodeGetInfoResponse>, Status> {
        Ok(Response::new(NodeGetInfoResponse {
            node_id: self.node_id.to_string(),
            max_volumes_per_node: 0,
            accessible_topology: None,
        }))
    }
}
