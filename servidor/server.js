const cors = require("cors");
const express = require("express");
const path = require("path");
require("./database");
const routes = require("./routes");

const app = express();
const port = process.env.PORT || 3000;

app.use(cors());
app.use(express.json());
app.use("/api", routes);
app.use(express.static(path.join(__dirname, "..", "app")));

app.listen(port, () => {
  console.log(`Servidor iniciado en http://localhost:${port}`);
});
