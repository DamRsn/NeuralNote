// The contact address is stored XOR-encoded (first byte is the key) so it never appears as plain text in the source.
document.querySelectorAll("[data-e]").forEach(function (el) {
  var hex = el.dataset.e;
  var key = parseInt(hex.slice(0, 2), 16);
  var address = "";
  for (var i = 2; i < hex.length; i += 2) address += String.fromCharCode(parseInt(hex.slice(i, i + 2), 16) ^ key);
  el.textContent = address;
});

// Show the version of the release marked Latest on GitHub; the link text stays "Latest release" if the request fails.
var latestVersion = document.getElementById("latest-version");
if (latestVersion && window.fetch) {
  fetch("https://api.github.com/repos/DamRsn/NeuralNote/releases/latest")
    .then(function (response) {
      return response.ok ? response.json() : null;
    })
    .then(function (release) {
      if (release && release.tag_name) latestVersion.textContent = "Version " + release.tag_name.replace(/^v/, "");
    })
    .catch(function () {});
}
