resource "google_compute_firewall" "ssh" {
  name = "ssh"
  allow {
    ports    = ["22"]
    protocol = "tcp"
  }
  direction     = "INGRESS"
  network       = google_compute_network.vpc_default.id
  priority      = 1000
  source_ranges = ["0.0.0.0/0"]
  target_tags   = [var.tag]
}

resource "google_compute_firewall" "iperf3" {
  name    = "iperf3"
  network = google_compute_network.vpc_default.id

  allow {
    protocol = "tcp"
    ports    = ["5201"]
  }
  source_ranges = ["0.0.0.0/0"]
}

resource "google_compute_firewall" "ping" {
  name    = "icmp"
  network = google_compute_network.vpc_default.id

  allow {
    protocol = "icmp"
  }
  source_ranges = ["0.0.0.0/0"]
}

resource "google_compute_firewall" "quic" {
  name    = "udp"
  network = google_compute_network.vpc_default.id

  allow {
    protocol = "udp"
    ports = ["443"]
  }
  source_ranges = ["0.0.0.0/0"]
}
