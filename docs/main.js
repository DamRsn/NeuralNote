// The contact address is stored XOR-encoded (first byte is the key) so it never appears as plain text in the source.
document.querySelectorAll("[data-e]").forEach(function (el) {
  var hex = el.dataset.e;
  var key = parseInt(hex.slice(0, 2), 16);
  var address = "";
  for (var i = 2; i < hex.length; i += 2) address += String.fromCharCode(parseInt(hex.slice(i, i + 2), 16) ^ key);
  el.textContent = address;
});
