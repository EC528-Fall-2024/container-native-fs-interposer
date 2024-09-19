use container_native_fs_interposer::{
    controller::ControllerPlugin,
    csi::v1::{
        controller_server::ControllerServer, identity_server::IdentityServer,
        node_server::NodeServer,
    },
    identity::IdentityPlugin,
    node::NodePlugin,
};
use tonic::transport::Server;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    Server::builder()
        .add_service(IdentityServer::new(IdentityPlugin {}))
        .add_service(ControllerServer::new(ControllerPlugin::new()))
        .add_service(NodeServer::new(NodePlugin::new()))
        .serve("[::1]:10000".parse()?)
        .await?;
    Ok(())
}
