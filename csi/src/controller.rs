use std::{collections::HashMap, sync::Mutex};

use crate::csi::v1::controller_server::Controller;
use crate::csi::v1::*;
use tonic::{Request, Response, Status};

pub struct ControllerPlugin {
    volumes: Mutex<HashMap<String, Option<CapacityRange>>>,
}

impl ControllerPlugin {
    pub fn new() -> Self {
        Self {
            volumes: Mutex::new(HashMap::new()),
        }
    }
}

#[tonic::async_trait]
impl Controller for ControllerPlugin {
    async fn create_volume(
        &self,
        request: Request<CreateVolumeRequest>,
    ) -> Result<Response<CreateVolumeResponse>, Status> {
        let mut volumes = self.volumes.lock().unwrap();
        let request = request.into_inner();
        if request.name.is_empty() {
            return Err(Status::invalid_argument("name is required"));
        }
        if request.volume_capabilities.is_empty() {
            return Err(Status::invalid_argument("volume_capabilites is required"));
        }
        if let Some(capacity_range) = volumes.get(&request.name) {
            if *capacity_range != request.capacity_range {
                return Err(Status::already_exists(
                    "volume of the same name with different capacity_range already exists",
                ));
            }
        };
        volumes.insert(request.name.clone(), request.capacity_range);
        Ok(Response::new(CreateVolumeResponse {
            volume: Some(Volume {
                capacity_bytes: 0,
                volume_id: request.name,
                volume_context: request.parameters,
                content_source: None,
                accessible_topology: vec![],
            }),
        }))
    }
    async fn delete_volume(
        &self,
        _: Request<DeleteVolumeRequest>,
    ) -> Result<Response<DeleteVolumeResponse>, Status> {
        Ok(Response::new(DeleteVolumeResponse {}))
    }
    async fn controller_publish_volume(
        &self,
        request: Request<ControllerPublishVolumeRequest>,
    ) -> Result<Response<ControllerPublishVolumeResponse>, Status> {
        unimplemented!()
    }
    async fn controller_unpublish_volume(
        &self,
        request: Request<ControllerUnpublishVolumeRequest>,
    ) -> Result<Response<ControllerUnpublishVolumeResponse>, Status> {
        unimplemented!()
    }
    async fn validate_volume_capabilities(
        &self,
        request: Request<ValidateVolumeCapabilitiesRequest>,
    ) -> Result<Response<ValidateVolumeCapabilitiesResponse>, Status> {
        unimplemented!()
    }
    async fn list_volumes(
        &self,
        request: Request<ListVolumesRequest>,
    ) -> Result<Response<ListVolumesResponse>, Status> {
        unimplemented!()
    }
    async fn get_capacity(
        &self,
        request: Request<GetCapacityRequest>,
    ) -> Result<Response<GetCapacityResponse>, Status> {
        unimplemented!()
    }
    async fn controller_get_capabilities(
        &self,
        _: Request<ControllerGetCapabilitiesRequest>,
    ) -> Result<Response<ControllerGetCapabilitiesResponse>, Status> {
        Ok(Response::new(ControllerGetCapabilitiesResponse {
            capabilities: vec![ControllerServiceCapability {
                r#type: Some(controller_service_capability::Type::Rpc(
                    controller_service_capability::Rpc {
                        r#type: controller_service_capability::rpc::Type::CreateDeleteVolume.into(),
                    },
                )),
            }],
        }))
    }
    async fn create_snapshot(
        &self,
        request: Request<CreateSnapshotRequest>,
    ) -> Result<Response<CreateSnapshotResponse>, Status> {
        unimplemented!()
    }
    async fn delete_snapshot(
        &self,
        request: Request<DeleteSnapshotRequest>,
    ) -> Result<Response<DeleteSnapshotResponse>, Status> {
        unimplemented!()
    }
    async fn list_snapshots(
        &self,
        request: Request<ListSnapshotsRequest>,
    ) -> Result<Response<ListSnapshotsResponse>, Status> {
        unimplemented!()
    }
    async fn controller_expand_volume(
        &self,
        request: Request<ControllerExpandVolumeRequest>,
    ) -> Result<Response<ControllerExpandVolumeResponse>, Status> {
        unimplemented!()
    }
    async fn controller_get_volume(
        &self,
        request: Request<ControllerGetVolumeRequest>,
    ) -> Result<Response<ControllerGetVolumeResponse>, Status> {
        unimplemented!()
    }
    async fn controller_modify_volume(
        &self,
        request: Request<ControllerModifyVolumeRequest>,
    ) -> Result<Response<ControllerModifyVolumeResponse>, Status> {
        unimplemented!()
    }
}
