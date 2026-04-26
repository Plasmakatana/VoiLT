mod load_balancer;
mod rate_limiter;

use pingora::prelude::*;
use pingora_core::services::background::background_service;
use std::{sync::Arc, time::Duration};
use pingora_load_balancing::{selection::RoundRobin, LoadBalancer};
use std::fs::File;
use std::io::{BufRead,BufReader};

fn load_upstreams_from_file(path: &str) -> Result<Vec<String>, std::io::Error> {
    let file = File::open(path)?;
    let reader = BufReader::new(file);

    let upstreams = reader
        .lines()
        .filter_map(|line| line.ok())
        .map(|line| line.trim().to_string())
        .filter(|line| !line.is_empty() && !line.starts_with('#'))
        .collect();

    Ok(upstreams)
}

fn main() {
    let mut my_server = Server::new(None).unwrap();
    my_server.bootstrap();
    let upstream_list = load_upstreams_from_file("upstreams.conf")
        .expect("Failed to read upstream config");
    let mut upstreams = LoadBalancer::try_from_iter(upstream_list).expect("Invalid upstream format");
    let hc = TcpHealthCheck::new();
    upstreams.set_health_check(hc);
    upstreams.health_check_frequency = Some(Duration::from_secs(1));

    let background = background_service("health check", upstreams);
    let upstreams = background.task();

    let mut lb = http_proxy_service(&my_server.configuration, load_balancer::LB(Arc::clone(&upstreams)));
    lb.add_tcp("0.0.0.0:10000");
    println!("Listening on: 0.0.0.0:10000");

    my_server.add_service(background);
    my_server.add_service(lb);
    my_server.run_forever();
}
