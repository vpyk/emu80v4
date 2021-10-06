Module["arguments"] = [];

const args = window.location.search.substr(1).trim().split("&");

for (const arg of args) {
 const pair = arg.split("=");
 if (pair.length == 1) Module["arguments"].push(pair[0]); else {
  Module["arguments"].push("--" + pair[0]);
  Module["arguments"].push(decodeURI(pair[1]));
 }
}
